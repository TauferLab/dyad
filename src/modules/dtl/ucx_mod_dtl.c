#include "ucx_mod_dtl.h"

#include "flux_b64.h"

#include <cstddef>

#define UCX_CHECK(status_code) return status_code != UCS_OK;

struct mod_request {
    int completed;
};
typedef struct mod_request mod_request_t;

static void dyad_mod_ucx_request_init(void *request)
{
    mod_request_t *real_request = (mod_request_t*)request;
    real_request->completed = 0;
}

static void dyad_ucx_send_handler(void *req, ucs_status_t status, void *ctx)
{
    mod_request_t *real_req = (mod_request_t*)req;
    real_req->completed = 1;
}

int dyad_mod_ucx_dtl_init(flux_t *h, bool debug, dyad_mod_ucx_dtl_t **dtl_handle)
{
    *dtl_handle = malloc(sizeof(struct dyad_mod_ucx_dtl));
    (*dtl_handle)->h = h;
    (*dtl_handle)->debug = debug;
    (*dtl_handle)->ucx_ctx = NULL;
    (*dtl_handle)->ucx_worker = NULL;
    (*dtl_handle)->curr_ep = NULL;
    (*dtl_handle)->curr_cons_addr = NULL;
    (*dtl_handle)->addr_len = 0;
    (*dtl_handle)->curr_comm_tag = 0;
    ucp_params_t ucp_params;
    ucp_worker_params_t worker_params;
    ucp_config_t *config;
    ucx_status_t status;
    status = ucp_config_read(NULL, NULL, &config);
    if (UCX_CHECK(status))
    {
        FLUX_LOG_ERR(h, "Could not read UCP config for data transport!\n");
        goto error;
    }
    ucp_params.field_mask = UCP_PARAMS_FIELD_FEATURES |
        UCP_PARAM_FIELD_REQUEST_SIZE |
        UCP_PARAM_FIELD_REQUEST_INIT;
    ucp_params.features = UCP_FEATURE_TAG |
        UCP_FEATURE_RMA |
        UCP_FEATURE_WAKEUP;
    ucp_params.request_size = sizeof(struct mod_request);
    ucp_params.request_init = dyad_mod_ucx_request_init;
    status = ucp_init(&ucp_params, config, (*dtl_handle)->ucx_ctx);
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
        goto error;
    }
    // Flux modules are single-threaded, so enable single-thread mode in UCX
    worker_params.field_mask = UCP_WORKER_PARAM_FIELD_THREAD_MODE;
    worker_params.thread_mode = UCS_THREAD_MODE_SINGLE;
    status = ucp_worker_create(
        (*dtl_handle)->ucx_ctx,
        &worker_params,
        &(*dtl_handle)->ucx_worker
    );
    if (UCX_CHECK(status))
    {
        FLUX_LOG_ERR(h, "Could not create UCP worker for data transport!\n");
        goto error;
    }
    return 0;

error:;
    dyad_mod_ucx_dtl_finalize(*dtl_handle);
    return -1;
}

int dyad_mod_ucx_dtl_rpc_unpack(dyad_mod_ucx_dtl_t *dtl_handle,
        flux_msg_t *packed_obj, char **upath)
{
    char* enc_addr;
    size_t enc_addr_len;
    int errcode = flux_request_unpack(packed_obj,
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
    dtl_handle->addr_len = base64_decoded_length(enc_addr_len);
    dtl_handle->curr_cons_addr = (ucp_address_t*) malloc(dtl_handle->addr_len);
    ssize_t decoded_len = base64_decode((char*)dtl_handle->curr_cons_addr, dtl_handle->addr_len, enc_addr, end_addr_len);
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

int dyad_mod_ucx_dtl_establish_connection(dyad_mod_ucx_dtl_t *dtl_handle)
{
    ucp_ep_params_t params;
    params.field_mask = UCP_EP_PARAM_REMOTE_ADDRESS |
        UCP_EP_PARAM_FIELD_ERR_HANDLING_MODE;
    params.address = dtl_handle->curr_cons_addr;
    params.err_mode = UCP_ERR_HANDLING_MODE_NONE; // TODO decide if we want to enable
                                                  // UCX endpoint error handling
    ucs_status_t status = ucp_ep_create(
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
    if (dtl_handle->curr_ep == NULL)
    {
        FLUX_LOG_INFO(dtl_handle->h, "UCP endpoint was not created prior to invoking send!\n");
        return 1;
    }
    ucp_request_param_t params;
    params.op_attr_mask = UCP_OP_ATTR_FIELD_CALLBACK;
    params.cb.send = dyad_ucx_send_handler;
    // TODO decide which tag send we want to use:
    //   * send: async send that is completed when the delayed send
    //           operation is completed
    //   * send_sync: async send that is completed when the message's
    //                tag is matched on the receiver
    ucs_status_ptr_t stat_ptr = ucp_tag_send_sync_nbx(
        dtl_handle->curr_ep,
        buf,
        buflen,
        dtl_handle->curr_comm_tag,
        params
    );
    ucs_status_t status;
    if (UCS_PTR_IS_ERR(stat_ptr))
    {
        status = UCS_PTR_STATUS(stat_ptr);
    }
    else if (UCS_PTR_IS_PTR(stat_ptr))
    {
        mod_request_t *req = (mod_request_t*)stat_ptr;
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
        FLUX_LOG_INFO(dtl_handle->h, "UCP Tag Send failed!\n");
        return -1;
    }
    return 0;
}

int dyad_mod_ucx_dtl_close_connection(dyad_mod_ucx_dtl_t *dtl_handle)
{
    if (dtl_handle != NULL)
    {
        ucs_status_t status;
        if (dtl_handle->curr_ep != NULL)
        {
            ucs_status_ptr_t stat_ptr;
            ucp_request_param_t close_params;
            close_params.op_attr_mask = UCP_OP_ATTR_FIELD_FLAGS;
            close_params.flags = 0; // TODO change if we decide to enable err handling mode
            stat_ptr = ucp_ep_close_nbx(dtl_handle->curr_ep, &close_params);
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
    return 0;
}

int dyad_mod_ucx_dtl_finalize(dyad_mod_dtl_t *dtl_handle)
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
