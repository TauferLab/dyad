#include "mercury_mod_dtl.h"
#include <sys/wait.h>

#define PROGRESS_TIMEOUT 100 // Timeout in milliseconds

struct mercury_cb_data {
    int completed;
    int good_type;
    na_return_t ret;
};

typedef struct mercury_cb_data mercury_cb_data_t;

int dyad_mod_dtl_recv_cb(const struct na_cb_info *callback_info)
{
    mercury_cb_data_t *user_data = (mercury_cb_data_t*) (callback_info->arg);
    if (callback_info->type == NA_CB_RECV_EXPECTED) {
        user_data->good_type = 1;
    } else {
        user_data->good_type = 0;
    }
    user_data->ret = callback_info->ret;
    if (callback_info->ret == NA_SUCCESS) {
        user_data->completed = 1;
    }
}

int wait_on_mercury_op(dyad_mod_mercury_dtl_t *dtl_handle,
        mercury_cb_data_t *cb_data)
{
    na_return_t ret_code = NA_SUCCESS;
    unsigned int count = 0;
    while (!cb_data->completed) {
        count = 0;
        do {
            ret_code = NA_Trigger(
                dtl_handle->mercury_ctx,
                PROGRESS_TIMEOUT,
                1,
                NULL,
                &count
            );
        } while ((ret_code == NA_SUCCESS) && count && !cb_data->completed);
        if (ret_code != NA_SUCCESS) {
            FLUX_LOG_ERR (dtl_handle->h, "Could not execute event in Mercury (NA_Trigger)\n");
        }
        ret_code = NA_Progress(
            dtl_handle->mercury_class,
            dtl_handle->mercury_ctx,
            PROGRESS_TIMEOUT
        );
        if (ret_code == NA_TIMEOUT) {
            FLUX_LOG_ERR (dtl_handle->h, "Timeout in Mercury while progressing progress queue\n");
            return -1;
        } else if (ret_code != NA_SUCCESS) {
            FLUX_LOG_ERR (dtl_handle->h, "A non-timeout error occured in NA_Progress\n");
            return -1;
        }
    }
    return 0;
}

int dyad_mod_mercury_dtl_init(flux_t *h, bool debug, dyad_mod_mercury_dtl_t **dtl_handle)
{
    na_return_t ret_code = NA_SUCCESS;
    char mercury_info_str[8] = "ofi+tcp";
    *dtl_handle = (dyad_mod_mercury_dtl_t*) malloc(sizeof(struct dyad_mod_mercury_dtl));
    if (*dtl_handle == NULL) {
        FLUX_LOG_ERR (h, "Could not allocate context for Mercury DTL\n");
        return -1;
    }
    (*dtl_handle)->h = h;
    (*dtl_handle)->mercury_class = NA_Initialize(
        mercury_info_str,
        false
    );
    if ((*dtl_handle)->mercury_class == NULL) {
        FLUX_LOG_ERR (h, "Could not initialize Mercury class\n");
        goto hg_init_error;
    }
    (*dtl_handle)->mercury_ctx = NA_Context_create(
        (*dtl_handle)->mercury_class
    );
    if ((*dtl_handle)->mercury_ctx == NULL) {
        FLUX_LOG_ERR (h, "Could not initialize Mercury context\n");
        goto hg_init_error;
    }
    ret_code = NA_Addr_self(
        (*dtl_handle)->mercury_class,
        (*dtl_handle)->mercury_addr
    );
    if (ret_code != NA_SUCCESS) {
        FLUX_LOG_ERR (h, "Could not get Mercury address\n");
        goto hg_init_error;
    }
    (*dtl_handle)->rpc_msg = NULL;
    (*dtl_handle)->mercury_tag = 0;
    (*dtl_handle)->consumer_addr = NULL;
    FLUX_LOG_INFO (h, "Mercury initialization successful\n");
    return 0;

hg_init_error:;
    dyad_mod_mercury_dtl_finalize(*dtl_handle);
    return -1;
}

int dyad_mod_mercury_dtl_rpc_unpack(dyad_mod_mercury_dtl_t *dtl_handle,
        const flux_msg_t *packed_obj, char **upath)
{
    na_return_t ret_code = NA_SUCCESS;
    char* enc_addr = NULL;
    size_t enc_addr_len = 0;
    int errcode = 0;
    uint32_t tag_prod = 0;
    uint32_t tag_cons = 0;
    ssize_t decoded_len = 0;
    FLUX_LOG_INFO (dtl_handle->h, "Unpacking RPC payload\n");
    errcode = flux_request_unpack(packed_obj,
        NULL,
        "{s:s, s:i, s:i, s:s%}",
        "upath",
        upath,
        "tag_prod",
        &tag_prod,
        "tag_cons",
        &tag_cons,
        "mercury_addr",
        &enc_addr,
        &enc_addr_len
    );
    if (errcode < 0)
    {
        FLUX_LOG_ERR(dtl_handle->h, "Could not unpack Flux message from consumer!\n");
        return -1;
    }
    dtl_handle->mercury_tag = (na_tag_t)tag_prod | (na_tag_t)tag_cons;
    FLUX_LOG_INFO (dtl_handle->h, "Obtained upath from RPC payload: %s\n", upath);
    FLUX_LOG_INFO (dtl_handle->h, "Obtained Mercury tag from RPC payload: %lu\n", dtl_handle->mercury_tag);
    FLUX_LOG_INFO (dtl_handle->h, "Decoding consumer Mercury address\n");
    ret_code = NA_Addr_deserialize(
        dtl_handle->mercury_class,
        dtl_handle->consumer_addr,
        (const void*) enc_addr,
        enc_addr_len
    );
    if (ret_code != NA_SUCCESS) {
        FLUX_LOG_ERR (dtl_handle->h, "Cannot deserialize the consumer's Mercury address\n");
        return -1;
    }
    return 0;
}

int dyad_mod_mercury_dtl_rpc_respond (dyad_mod_mercury_dtl_t *dtl_handle,
        const flux_msg_t *orig_msg)
{
    dtl_handle->rpc_msg = orig_msg;
    return 0;
}

int dyad_mod_mercury_dtl_establish_connection(dyad_mod_mercury_dtl_t *dtl_handle)
{
    return 0;
}

int dyad_mod_mercury_dtl_send(dyad_mod_mercury_dtl_t *dtl_handle, void *buf, size_t buflen)
{
    na_return_t ret_code = NA_SUCCESS;
    na_mem_handle_t *local_mem_handle = NULL;
    mercury_cb_data_t *cb_arg = NULL;
    void *recv_msg = NULL;
    void *plugin_data = NULL;
    na_op_id_t *recv_id = NULL;
    int rc = 0;
    size_t addr_size = 0;
    void *serialized_addr = NULL;
    size_t handle_size = 0;
    void *serialized_handle = NULL;
    int ack_response = 0;
    bool error_occured = false;
    bool mem_registered = false;

    addr_size = NA_Addr_get_serialize_size(
        dtl_handle->mercury_class,
        *(dtl_handle->mercury_addr)
    );
    serialized_addr = malloc(addr_size);
    if (serialized_addr == NULL) {
        FLUX_LOG_ERR (dtl_handle->h, "Cannot allocate memory for serialized address\n");
        error_occured = true;
        goto send_cleanup;
    }
    ret_code = NA_Addr_serialize(
        dtl_handle->mercury_class,
        serialized_addr,
        addr_size,
        *(dtl_handle->mercury_addr)
    );
    if (ret_code != NA_SUCCESS) {
        FLUX_LOG_ERR (dtl_handle->h, "Cannot serialize Mercury address\n");
        error_occured = true;
        goto send_cleanup;
    }
    ret_code = NA_Mem_handle_create(
        dtl_handle->mercury_class,
        buf,
        buflen,
        NA_MEM_READWRITE, // TOOD consider NA_MEM_READ_ONLY
        local_mem_handle
    );
    if (ret_code != NA_SUCCESS) {
        FLUX_LOG_ERR (dtl_handle->h, "Could not create local Mercury memory handle\n");
        error_occured = true;
        goto send_cleanup;
    }
    ret_code = NA_Mem_register(
        dtl_handle->mercury_class,
        *local_mem_handle
    );
    if (ret_code != NA_SUCCESS) {
        FLUX_LOG_ERR (dtl_handle->h, "Could not register local memory handle with Mercury\n");
        error_occured = true;
        goto send_cleanup;
    }
    mem_registered = true;
    handle_size = NA_Mem_handle_get_serialize_size(
        dtl_handle->mercury_class,
        *local_mem_handle
    );
    serialized_handle = malloc(handle_size);
    if (serialized_handle == NULL) {
        FLUX_LOG_ERR (dtl_handle->h, "Cannot allocate memory for serialzied handle\n");
        error_occured = true;
        goto send_cleanup;
    }
    ret_code = NA_Mem_handle_serialize(
        dtl_handle->mercury_class,
        serialized_handle,
        handle_size,
        *local_mem_handle
    );
    if (ret_code != NA_SUCCESS) {
        FLUX_LOG_ERR (dtl_handle->h, "Cannot serialize Mercury memory handle\n");
        error_occured = true;
        goto send_cleanup;
    }
    cb_arg = (mercury_cb_data_t*) malloc(sizeof(struct mercury_cb_data));
    cb_arg->completed = 0;
    cb_arg->good_type = 0;
    cb_arg->ret = NA_SUCCESS;
    recv_msg = NA_Msg_buf_alloc(
        dtl_handle->mercury_class,
        sizeof(int),
        &plugin_data
    );
    if (recv_msg == NULL) {
        FLUX_LOG_ERR (dtl_handle->h, "Cannot allocate response message\n");
        error_occured = true;
        goto send_cleanup;
    }
    recv_id = NA_Op_create(
        dtl_handle->mercury_class
    );
    ret_code = NA_Msg_recv_expected(
        dtl_handle->mercury_class,
        dtl_handle->mercury_ctx,
        dyad_mod_dtl_recv_cb,
        cb_arg,
        (void*) recv_msg,
        sizeof(int),
        plugin_data,
        *(dtl_handle->consumer_addr),
        0,
        dtl_handle->mercury_tag,
        recv_id
    );
    if (ret_code != NA_SUCCESS) {
        FLUX_LOG_ERR (dtl_handle->h, "Pre-post of response recv through Mercury failed\n");
        error_occured = true;
        goto send_cleanup;
    }
    rc = flux_respond_pack(
        dtl_handle->h,
        dtl_handle->rpc_msg,
        "{s:I, s:s%, s:s%}",
        "data_size",
        buflen,
        "mem_handle",
        serialized_handle, handle_size,
        "module_addr",
        serialized_addr, addr_size
    );
    if (rc < 0) {
        FLUX_LOG_ERR (dtl_handle->h, "Cannot send memory and address information to consumer\n");
        error_occured = true;
        goto send_cleanup;
    }
    rc = wait_on_mercury_op(dtl_handle, cb_arg);
    if (rc != 0) {
        FLUX_LOG_ERR (dtl_handle->h, "Failed to wait for ACK from consumer\n");
        error_occured = true;
        goto send_cleanup;
    }
    memcpy(&ack_response, recv_msg, sizeof(int));
    if (ack_response == 0) {
        FLUX_LOG_ERR (dtl_handle->h, "An error occured on the consumer size (BAD ACK)\n");
        error_occured = true;
        goto send_cleanup;
    }
    FLUX_LOG_INFO (dtl_handle->h, "Received a postive ACK\n");
    error_occured = false;

send_cleanup:;
    if (serialized_addr != NULL) {
        free(serialized_addr);
        serialized_addr = NULL;
    }
    if (serialized_handle != NULL) {
        free(serialized_handle);
        serialized_handle = NULL;
    }
    if (local_mem_handle != NULL) {
        if (mem_registered) {
            ret_code = NA_Mem_deregister(
                dtl_handle->mercury_class,
                *local_mem_handle
            );
            if (ret_code != NA_SUCCESS)
                FLUX_LOG_ERR (dtl_handle->h, "Could not deregister local memory from Mercury\n");
        }
        NA_Mem_handle_free(
            dtl_handle->mercury_class,
            *local_mem_handle
        );
        local_mem_handle = NULL;
    }
    if (cb_arg != NULL) {
        free(cb_arg);
        cb_arg = NULL;
    }
    if (recv_msg != NULL || plugin_data != NULL) {
        NA_Msg_buf_free(
            dtl_handle->mercury_class,
            recv_msg,
            plugin_data
        );
        recv_msg = NULL;
        plugin_data = NULL;
    }
    if (recv_id != NULL) {
        NA_Op_destroy(dtl_handle->mercury_class, recv_id);
        recv_id = NULL;
    }
    if (error_occured)
        return -1;
    return 0;
}

int dyad_mod_mercury_dtl_close_connection(dyad_mod_mercury_dtl_t *dtl_handle)
{
    dtl_handle->rpc_msg = NULL;
    dtl_handle->mercury_tag = 0;
    if (dtl_handle->consumer_addr != NULL) {
        NA_Addr_free(
            dtl_handle->mercury_class,
            *(dtl_handle->consumer_addr)
        );
        dtl_handle->consumer_addr = NULL;
    }
    return 0;
}

int dyad_mod_mercury_dtl_finalize(dyad_mod_mercury_dtl_t *dtl_handle)
{
    if (dtl_handle != NULL)
    {
        dyad_mod_mercury_dtl_close_connection(dtl_handle);
        if (dtl_handle->mercury_addr != NULL)
        {
            NA_Addr_free(
                dtl_handle->mercury_class,
                *(dtl_handle->mercury_addr)
            );
            dtl_handle->mercury_addr = NULL;
        }
        if (dtl_handle->mercury_ctx != NULL)
        {
            NA_Context_destroy(
                dtl_handle->mercury_class,
                dtl_handle->mercury_ctx
            );
            dtl_handle->mercury_ctx = NULL;
        }
        if (dtl_handle->mercury_class != NULL) {
            NA_Finalize(dtl_handle->mercury_class);
            dtl_handle->mercury_class = NULL;
        }
        dtl_handle->h = NULL;
        free(dtl_handle);
        dtl_handle = NULL;
    }
    return 0;
}
