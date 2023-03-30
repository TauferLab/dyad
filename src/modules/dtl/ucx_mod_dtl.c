#include "ucx_mod_dtl.h"

#include "base64.h"

// Get base64_maps_rfc4648 from flux-core
extern const base64_maps_t base64_maps_rfc4648;

#define UCX_CHECK(status_code) status_code != UCS_OK

#if !defined(UCP_API_VERSION)
#error Due to UCP API changes, we must be able to determine the version of UCP! \
    Please use a version of UCX with the UCP_API_VERSION macro defined!
#endif

struct mod_request {
    int completed;
};
typedef struct mod_request mod_request_t;

static void dyad_mod_ucx_request_init(void *request)
{
    mod_request_t *real_request = (mod_request_t*)request;
    real_request->completed = 0;
}

#if UCP_API_VERSION >= UCP_VERSION(1, 10)
static void dyad_ucx_send_handler(void *req, ucs_status_t status, void *ctx)
#else
static void dyad_ucx_send_handler(void *req, ucs_status_t status)
#endif
{
    mod_request_t *real_req = (mod_request_t*)req;
    real_req->completed = 1;
}

int dyad_mod_ucx_dtl_init(flux_t *h, bool debug, dyad_mod_ucx_dtl_t **dtl_handle)
{
    ucp_params_t ucp_params;
    ucp_worker_params_t worker_params;
    ucp_config_t *config = NULL;
    ucs_status_t status = UCS_OK;
    *dtl_handle = malloc(sizeof(struct dyad_mod_ucx_dtl));
    (*dtl_handle)->h = h;
    (*dtl_handle)->debug = debug;
    (*dtl_handle)->ucx_ctx = NULL;
    (*dtl_handle)->ucx_worker = NULL;
    (*dtl_handle)->curr_ep = NULL;
    (*dtl_handle)->curr_cons_addr = NULL;
    (*dtl_handle)->addr_len = 0;
    (*dtl_handle)->curr_comm_tag = 0;
    FLUX_LOG_INFO (h, "Reading UCP config for DTL\n");
    status = ucp_config_read(NULL, NULL, &config);
    if (UCX_CHECK(status))
    {
        FLUX_LOG_ERR(h, "Could not read UCP config for data transport!\n");
        goto ucx_init_error;
    }
    ucp_params.field_mask = UCP_PARAM_FIELD_FEATURES |
        UCP_PARAM_FIELD_REQUEST_SIZE |
        UCP_PARAM_FIELD_REQUEST_INIT;
    ucp_params.features = UCP_FEATURE_TAG |
        UCP_FEATURE_RMA |
        UCP_FEATURE_WAKEUP;
    ucp_params.request_size = sizeof(struct mod_request);
    ucp_params.request_init = dyad_mod_ucx_request_init;
    FLUX_LOG_INFO (h, "Initializing UCX\n");
    status = ucp_init(&ucp_params, config, &((*dtl_handle)->ucx_ctx));
    if (debug)
    {
        ucp_config_print(
            config,
            stderr,
            "UCX Configuration",
            UCS_CONFIG_PRINT_CONFIG
        );
    }
    ucp_config_release(config);
    if (UCX_CHECK(status))
    {
        FLUX_LOG_ERR(h, "Could not initialize UCX for data transport!\n");
        goto ucx_init_error;
    }
    // Flux modules are single-threaded, so enable single-thread mode in UCX
    worker_params.field_mask = UCP_WORKER_PARAM_FIELD_THREAD_MODE |
        UCP_WORKER_PARAM_FIELD_EVENTS;
    worker_params.thread_mode = UCS_THREAD_MODE_SINGLE;
    worker_params.events = UCP_WAKEUP_TAG_SEND;
    FLUX_LOG_INFO (h, "Creating UCP worker\n");
    status = ucp_worker_create(
        (*dtl_handle)->ucx_ctx,
        &worker_params,
        &(*dtl_handle)->ucx_worker
    );
    if (UCX_CHECK(status))
    {
        FLUX_LOG_ERR(h, "Could not create UCP worker for data transport!\n");
        goto ucx_init_error;
    }
    FLUX_LOG_INFO (h, "UCX initialization successful\n");
    return 0;

ucx_init_error:;
    dyad_mod_ucx_dtl_finalize(*dtl_handle);
    return -1;
}

int dyad_mod_ucx_dtl_rpc_unpack(dyad_mod_ucx_dtl_t *dtl_handle,
        const flux_msg_t *packed_obj, char **upath)
{
    char* enc_addr = NULL;
    size_t enc_addr_len = 0;
    int errcode = 0;
    ssize_t decoded_len = 0;
    FLUX_LOG_INFO (dtl_handle->h, "Unpacking RPC payload\n");
    errcode = flux_request_unpack(packed_obj,
        NULL,
        "{s:s, s:I, s:s%}",
        "upath",
        upath,
        "tag",
        dtl_handle->curr_comm_tag,
        "ucx_addr",
        &enc_addr,
        &enc_addr_len
    );
    if (errcode < 0)
    {
        FLUX_LOG_ERR(dtl_handle->h, "Could not unpack Flux message from consumer!\n");
        return -1;
    }
    FLUX_LOG_INFO (dtl_handle->h, "Obtained upath from RPC payload: %s\n", upath);
    FLUX_LOG_INFO (dtl_handle->h, "Obtained UCP tag from RPC payload: %lu\n", dtl_handle->curr_comm_tag);
    FLUX_LOG_INFO (dtl_handle->h, "Decoding consumer UCP address using base64\n");
    dtl_handle->addr_len = base64_decoded_length(enc_addr_len);
    dtl_handle->curr_cons_addr = (ucp_address_t*) malloc(dtl_handle->addr_len);
    decoded_len = base64_decode_using_maps (&base64_maps_rfc4648,
            (char*)dtl_handle->curr_cons_addr, dtl_handle->addr_len,
            enc_addr, enc_addr_len);
    if (decoded_len < 0)
    {
        // TODO log error
        free(dtl_handle->curr_cons_addr);
        dtl_handle->curr_cons_addr = NULL;
        dtl_handle->addr_len = 0;
        return -1;
    }
    return 0;
}

int dyad_mod_ucx_dtl_rpc_respond (dyad_mod_ucx_dtl_t *dtl_handle,
        const flux_msg_t *orig_msg)
{
    return 0;
}

int dyad_mod_ucx_dtl_establish_connection(dyad_mod_ucx_dtl_t *dtl_handle)
{
    ucp_ep_params_t params;
    ucs_status_t status = UCS_OK;
    params.field_mask = UCP_EP_PARAM_FIELD_REMOTE_ADDRESS |
        UCP_EP_PARAM_FIELD_ERR_HANDLING_MODE;
    params.address = dtl_handle->curr_cons_addr;
    params.err_mode = UCP_ERR_HANDLING_MODE_NONE; // TODO decide if we want to enable
                                                  // UCX endpoint error handling
    FLUX_LOG_INFO (dtl_handle->h, "Create UCP endpoint for communication with consumer\n");
    status = ucp_ep_create(
        dtl_handle->ucx_worker,
        &params,
        &dtl_handle->curr_ep
    );
    if (UCX_CHECK(status))
    {
        return -1;
    }
    if (dtl_handle->debug)
    {
        ucp_ep_print_info(dtl_handle->curr_ep, stderr);
    }
    return 0;
}

int dyad_mod_ucx_dtl_send(dyad_mod_ucx_dtl_t *dtl_handle, void *buf, size_t buflen)
{
    ucs_status_ptr_t stat_ptr;
    ucs_status_t status = UCS_OK;
    mod_request_t *req = NULL;
    if (dtl_handle->curr_ep == NULL)
    {
        FLUX_LOG_INFO(dtl_handle->h, "UCP endpoint was not created prior to invoking send!\n");
        return 1;
    }
    // ucp_tag_send_sync_nbx is the prefered version of this send since UCX 1.9
    // However, some systems (e.g., Lassen) may have an older verison
    // This conditional compilation will use ucp_tag_send_sync_nbx if using UCX 1.9+,
    // and it will use the deprecated ucp_tag_send_sync_nb if using UCX < 1.9.
#if UCP_API_VERSION >= UCP_VERSION(1, 10)
    ucp_request_param_t params;
    params.op_attr_mask = UCP_OP_ATTR_FIELD_CALLBACK;
    params.cb.send = dyad_ucx_send_handler;
    FLUX_LOG_INFO (dtl_handle->h, "Sending data to consumer with ucp_tag_send_sync_nbx\n");
    stat_ptr = ucp_tag_send_sync_nbx(
        dtl_handle->curr_ep,
        buf,
        buflen,
        dtl_handle->curr_comm_tag,
        &params
    );
#else
    FLUX_LOG_INFO (dtl_handle->h, "Sending data to consumer with ucp_tag_send_sync_nb\n");
    stat_ptr = ucp_tag_send_sync_nb(
        dtl_handle->curr_ep,
        buf,
        buflen,
        UCP_DATATYPE_CONTIG,
        dtl_handle->curr_comm_tag,
        dyad_ucx_send_handler
    );
#endif
    FLUX_LOG_INFO (dtl_handle->h, "Wait for send to complete\n");
    if (UCS_PTR_IS_ERR(stat_ptr))
    {
        status = UCS_PTR_STATUS(stat_ptr);
    }
    else if (UCS_PTR_IS_PTR(stat_ptr))
    {
        req = (mod_request_t*)stat_ptr;
        while (req->completed != 0)
        {
            ucp_worker_progress(dtl_handle->ucx_worker);
        }
        req->completed = 0;
        status = ucp_request_check_status(req);
        ucp_request_free(req);
    }
    else
    {
        status = UCS_OK;
    }
    if (UCX_CHECK(status))
    {
        FLUX_LOG_ERR (dtl_handle->h, "UCP Tag Send failed!\n");
        return -1;
    }
    FLUX_LOG_INFO (dtl_handle->h, "Data send with UCP succeeded\n");
    return 0;
}

int dyad_mod_ucx_dtl_close_connection(dyad_mod_ucx_dtl_t *dtl_handle)
{
    ucs_status_t status = UCS_OK;
    ucs_status_ptr_t stat_ptr;
    if (dtl_handle != NULL)
    {
        if (dtl_handle->curr_ep != NULL)
        {
            // ucp_tag_send_sync_nbx is the prefered version of this send since UCX 1.9
            // However, some systems (e.g., Lassen) may have an older verison
            // This conditional compilation will use ucp_tag_send_sync_nbx if using UCX 1.9+,
            // and it will use the deprecated ucp_tag_send_sync_nb if using UCX < 1.9.
            FLUX_LOG_INFO (dtl_handle->h, "Start async closing of UCP endpoint\n");
#if UCP_API_VERSION >= UCP_VERSION(1, 10)
            ucp_request_param_t close_params;
            close_params.op_attr_mask = UCP_OP_ATTR_FIELD_FLAGS;
            close_params.flags = 0; // TODO change if we decide to enable err handling mode
            stat_ptr = ucp_ep_close_nbx(dtl_handle->curr_ep, &close_params);
#else
            // TODO change to FORCE if we decide to enable err handleing mode
            stat_ptr = ucp_ep_close_nb(dtl_handle->curr_ep, UCP_EP_CLOSE_MODE_FLUSH);
#endif
            FLUX_LOG_INFO (dtl_handle->h, "Wait for endpoint closing to finish\n");
            if (stat_ptr != NULL)
            {
                // Endpoint close is in-progress.
                // Wait until finished
                if (UCS_PTR_IS_PTR(stat_ptr))
                {
                    do {
                        ucp_worker_progress(dtl_handle->ucx_worker);
                        status = ucp_request_check_status(stat_ptr);
                    } while (status == UCS_INPROGRESS);
                    ucp_request_free(stat_ptr);
                }
                // An error occurred during endpoint closure
                // However, the endpoint can no longer be used
                // Get the status code for reporting
                else
                {
                    status = UCS_PTR_STATUS(stat_ptr);
                }
                if (UCX_CHECK(status))
                {
                    FLUX_LOG_ERR(dtl_handle->h, "Could not successfully close Endpoint! However, endpoint was released.\n");
                }
            }
            dtl_handle->curr_ep = NULL;
        }
        if (dtl_handle->curr_cons_addr != NULL)
        {
            free(dtl_handle->curr_cons_addr);
            dtl_handle->curr_cons_addr = NULL;
            dtl_handle->addr_len = 0;
        }
        dtl_handle->curr_comm_tag = 0;
    }
    FLUX_LOG_INFO (dtl_handle->h, "UCP endpoint close successful\n");
    return 0;
}

int dyad_mod_ucx_dtl_finalize(dyad_mod_ucx_dtl_t *dtl_handle)
{
    if (dtl_handle != NULL)
    {
        if (dtl_handle->curr_cons_addr != NULL || dtl_handle->curr_ep != NULL)
        {
            dyad_mod_ucx_dtl_close_connection(dtl_handle);
        }
        if (dtl_handle->ucx_worker != NULL)
        {
            ucp_worker_destroy(dtl_handle->ucx_worker);
            dtl_handle->ucx_worker = NULL;
        }
        if (dtl_handle->ucx_ctx != NULL)
        {
            ucp_cleanup(dtl_handle->ucx_ctx);
            dtl_handle->ucx_ctx = NULL;
        }
        dtl_handle->h = NULL;
        free(dtl_handle);
        dtl_handle = NULL;
    }
    return 0;
}
