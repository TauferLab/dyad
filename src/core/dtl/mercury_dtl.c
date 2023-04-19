#include "mercury_dtl.h"

#ifdef __cplusplus
#include <cstdint>
#include <cstdio>
#include <cstring>
#else
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#endif

#define PROGRESS_TIMEOUT 100 // Timout in milliseconds

struct mercury_cb_data {
    int completed;
    int good_type;
    na_return_t ret;
};

typedef struct mercury_cb_data mercury_cb_data_t;

int dyad_dtl_get_cb(const struct na_cb_info *callback_info)
{
    mercury_cb_data_t *user_data = (mercury_cb_data_t*) (callback_info->arg);
    if (callback_info->type == NA_CB_GET) {
        user_data->good_type = 1;
    } else {
        user_data->good_type = 0;
    }
    user_data->ret = callback_info->ret;
    if (callback_info->ret == NA_SUCCESS) {
        user_data->completed = 1;
    } else {
        user_data->completed = 0;
    }
    return 0;
}

int dyad_dtl_send_cb(const struct na_cb_info *callback_info)
{
    mercury_cb_data_t *user_data = (mercury_cb_data_t*) (callback_info->arg);
    if (callback_info->type == NA_CB_SEND_EXPECTED) {
        user_data->good_type = 1;
    } else {
        user_data->good_type = 0;
    }
    user_data->ret = callback_info->ret;
    if (callback_info->ret == NA_SUCCESS) {
        user_data->completed = 1;
    }
    return 0;
}

int wait_on_mercury_op(dyad_dtl_mercury_t *dtl_handle,
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

dyad_rc_t dyad_dtl_mercury_init(flux_t *h, const char *kvs_namespace,
        bool debug, dyad_dtl_mercury_t **dtl_handle)
{
    // TODO add ability to set network lib via env var
    // char *e = NULL;
    // char *mercury_info_str = NULL;
    na_return_t ret_code = NA_SUCCESS;

    // if ((e = getenv("DYAD_DTL_MERCURY_INFO_STRING"))) {
    //     mercury_info_str = e;
    // } else {
    //    mercury_info_str = "ucx+all";
    // }
    char mercury_info_str[8] = "ofi+verbs";
    *dtl_handle = (dyad_dtl_mercury_t*) malloc(sizeof(struct dyad_dtl_mercury));
    if (*dtl_handle == NULL) {
        FLUX_LOG_ERR (h, "Could not allocate Mercury DTL context\n");
        return DYAD_RC_SYSFAIL;
    }
    NA_Set_log_level("debug");
    (*dtl_handle)->h = h;
    (*dtl_handle)->kvs_namespace = kvs_namespace;
    (*dtl_handle)->mercury_class = NA_Initialize(
        mercury_info_str,
        false
    );
    if ((*dtl_handle)->mercury_class == NULL) {
        FLUX_LOG_ERR (h, "Could not initialize Mercury class\n");
        goto error;
    }
    (*dtl_handle)->mercury_ctx = NA_Context_create(
        (*dtl_handle)->mercury_class
    );
    if ((*dtl_handle)->mercury_ctx == NULL) {
        FLUX_LOG_ERR (h, "Could not initialize Mercury context\n");
        goto error;
    }
    ret_code = NA_Addr_self(
        (*dtl_handle)->mercury_class,
        (*dtl_handle)->mercury_addr
    );
    if (ret_code != NA_SUCCESS) {
        FLUX_LOG_ERR (h, "Could not get Mercury address (code: %d)\n", ret_code);
        goto error;
    }
    (*dtl_handle)->mercury_tag = 0;
    (*dtl_handle)->remote_mem_handle = NULL;
    (*dtl_handle)->remote_addr = NULL;
    (*dtl_handle)->data_size = 0;

    return DYAD_RC_OK;

error:;
    // If an error occured, finalize the DTL handle and
    // return a failing error code
    dyad_dtl_mercury_finalize(*dtl_handle);
    return DYAD_RC_HG_INIT_FAIL;
}

dyad_rc_t dyad_dtl_mercury_rpc_pack(dyad_dtl_mercury_t *dtl_handle, const char *upath,
        uint32_t producer_rank, json_t **packed_obj)
{
    size_t serialized_size = 0;
    void *serialized_addr = NULL;
    na_return_t ret_code = NA_SUCCESS;

    serialized_size = NA_Addr_get_serialize_size(
        dtl_handle->mercury_class,
        *(dtl_handle->mercury_addr)
    );
    serialized_addr = malloc(serialized_size);
    if (serialized_addr == NULL) {
        FLUX_LOG_ERR (dtl_handle->h, "Cannot allocate memory for serialized address\n");
        return DYAD_RC_SYSFAIL;
    }
    ret_code = NA_Addr_serialize(
        dtl_handle->mercury_class,
        serialized_addr,
        serialized_size,
        *(dtl_handle->mercury_addr)
    );
    if (ret_code != NA_SUCCESS) {
        FLUX_LOG_ERR (dtl_handle->h, "Cannot serialize Mercury address\n");
        free(serialized_addr);
        return DYAD_RC_HG_OP_FAIL;
    }
    FLUX_LOG_INFO (dtl_handle->h, "Creating Mercury tag for tag matching\n");
    uint32_t consumer_rank = 0;
    if (flux_get_rank(dtl_handle->h, &consumer_rank) < 0)
    {
        FLUX_LOG_ERR (dtl_handle->h, "Cannot get consumer rank\n");
        free(serialized_addr);
        return DYAD_RC_FLUXFAIL;
    }
    // na_tag_t is actually a uint32_t
    dtl_handle->mercury_tag = (na_tag_t)producer_rank | (na_tag_t)consumer_rank;
    // Use Jansson to pack the tag and UCX address into
    // the payload to be sent via RPC to the producer plugin
    FLUX_LOG_INFO (dtl_handle->h, "Packing RPC payload for Mercury DTL\n");
    *packed_obj = json_pack(
        "{s:s, s:i, s:i, s:s%}",
        "upath",
        upath,
        "tag_prod",
        (int) producer_rank,
        "tag_cons",
        (int) consumer_rank,
        "mercury_addr",
        serialized_addr, serialized_size
    );
    free(serialized_addr);
    // If the packing failed, log an error
    if (*packed_obj == NULL)
    {
        FLUX_LOG_ERR (dtl_handle->h, "Could not pack RPC for Mercury DTL\n");
        return DYAD_RC_BADPACK;
    }
    return DYAD_RC_OK;
}

dyad_rc_t dyad_dtl_mercury_recv_rpc_response(dyad_dtl_mercury_t *dtl_handle,
        flux_future_t *f)
{
    int rc = 0;
    char *serialized_addr = NULL;
    size_t addr_size = 0;
    char *serialized_handle = NULL;
    size_t handle_size = 0;
    na_return_t ret_code = NA_SUCCESS;
    rc = flux_rpc_get_unpack (
        f,
        "{s:I, s:s%, s:s%}",
        "data_size",
        &(dtl_handle->data_size),
        "mem_handle",
        &serialized_handle, &handle_size,
        "module_addr",
        &serialized_addr, &addr_size
    );
    if (rc < 0) {
        FLUX_LOG_ERR (dtl_handle->h, "Cannot unpack RPC response for Mercury DTL\n");
        return DYAD_RC_FLUXFAIL;
    }
    flux_future_reset(f);
    ret_code = NA_Addr_deserialize(
        dtl_handle->mercury_class,
        dtl_handle->remote_addr,
        (const void*) serialized_addr,
        addr_size
    );
    if (ret_code != NA_SUCCESS) {
        FLUX_LOG_ERR (dtl_handle->h, "Cannot deserialize module's Mercury address\n");
        return DYAD_RC_HG_OP_FAIL;
    }
    ret_code = NA_Mem_handle_deserialize (
        dtl_handle->mercury_class,
        dtl_handle->remote_mem_handle,
        (const void*) serialized_handle,
        handle_size
    );
    if (ret_code != NA_SUCCESS) {
        FLUX_LOG_ERR (dtl_handle->h, "Cannot deserialize module's memory handle\n");
        return DYAD_RC_HG_OP_FAIL;
    }
    return DYAD_RC_OK;
}

dyad_rc_t dyad_dtl_mercury_establish_connection(dyad_dtl_mercury_t *dtl_handle)
{
    return DYAD_RC_OK;
}

dyad_rc_t dyad_dtl_mercury_recv(dyad_dtl_mercury_t *dtl_handle,
        void **buf, size_t *buflen)
{
    int rc = 0;
    int ack_response = 0;
    bool mem_registered = false;
    void *send_msg = NULL;
    void *plugin_data = NULL;
    dyad_rc_t dyad_ret_code = DYAD_RC_OK;
    na_mem_handle_t *local_mem_handle = NULL;
    na_op_id_t *get_id = NULL;
    na_op_id_t *send_id = NULL;
    na_return_t ret_code = NA_SUCCESS;
    mercury_cb_data_t *cb_arg = NULL;
    // Allocate buffer for data
    *buflen = dtl_handle->data_size;
    *buf = malloc(*buflen);
    // If allocation failed, error out
    if (*buf == NULL) {
        FLUX_LOG_ERR (dtl_handle->h, "Could not allocate memory for file recv\n");
        dyad_ret_code = DYAD_RC_SYSFAIL;
        goto send_ack;
    }
    // Create a Mercury memory handle for the data
    ret_code = NA_Mem_handle_create(
        dtl_handle->mercury_class,
        *buf,
        *buflen,
        NA_MEM_READWRITE, // TODO consider NA_MEM_READ_ONLY
        local_mem_handle
    );
    // If memory handle creation failed, error out
    if (ret_code != NA_SUCCESS) {
        FLUX_LOG_ERR (dtl_handle->h, "Could not create local Mercury memory handle\n");
        dyad_ret_code = DYAD_RC_HG_OP_FAIL;
        goto send_ack;
    }
    // Register memory handle
    ret_code = NA_Mem_register(
        dtl_handle->mercury_class,
        *local_mem_handle
    );
    // If handle registration failed, error out
    if (ret_code != NA_SUCCESS) {
        FLUX_LOG_ERR (dtl_handle->h, "Could not register local memory handle with Mercury\n");
        dyad_ret_code = DYAD_RC_HG_OP_FAIL;
        goto send_ack;
    }
    mem_registered = true;
    // Create Op ID for the Get
    get_id = NA_Op_create(
        dtl_handle->mercury_class
    );
    // If ID creation failed, error out
    if (get_id == NULL) {
        FLUX_LOG_ERR (dtl_handle->h, "Cannot create Op ID for Mercury Get\n");
        dyad_ret_code = DYAD_RC_HG_OP_FAIL;
        goto send_ack;
    }
    // Create a mercury_cb_data_t object to track completion of communication operation
    cb_arg = (mercury_cb_data_t*) malloc(sizeof(struct mercury_cb_data));
    cb_arg->completed = 0;
    cb_arg->good_type = 0;
    cb_arg->ret = NA_SUCCESS;
    // Get data
    ret_code = NA_Get(
        dtl_handle->mercury_class,
        dtl_handle->mercury_ctx,
        dyad_dtl_get_cb,
        (void*)cb_arg,
        *local_mem_handle,
        0,
        *(dtl_handle->remote_mem_handle),
        0,
        dtl_handle->data_size,
        *(dtl_handle->remote_addr),
        0,
        get_id
    );
    // If Get operation failed immediately, error out
    if (ret_code != NA_SUCCESS) {
        FLUX_LOG_ERR (dtl_handle->h, "Cannot get data from module through Mercury\n");
        dyad_ret_code = DYAD_RC_HG_COMM_FAIL;
        goto send_ack;
    }
    // Wait for Get operation to complete, and error out if it fails
    rc = wait_on_mercury_op(dtl_handle, cb_arg);
    if (rc != 0) {
        FLUX_LOG_ERR (dtl_handle->h, "Failed to wait for data fetching (NA_Get) to complete\n");
        // TODO consider calling NA_Cancel here if needed
        dyad_ret_code = DYAD_RC_HG_COMM_FAIL;
        goto send_ack;
    }
    free(cb_arg);
    cb_arg = NULL;
    dyad_ret_code = DYAD_RC_OK;

send_ack:;
    // Setup data for notifying module of success
    cb_arg = (mercury_cb_data_t*) malloc(sizeof(struct mercury_cb_data));
    cb_arg->completed = 0;
    cb_arg->good_type = 0;
    cb_arg->ret = NA_SUCCESS;
    ack_response = (dyad_ret_code == DYAD_RC_OK) ? 1 : 0;
    send_msg = NA_Msg_buf_alloc(
        dtl_handle->mercury_class,
        sizeof(int),
        &plugin_data
    );
    if (send_msg == NULL) {
        FLUX_LOG_ERR (dtl_handle->h, "Cannot allocate response message (ACK: %d)\n", ack_response);
        dyad_ret_code = DYAD_RC_HG_OP_FAIL;
        goto recv_cleanup;
    }
    memcpy(send_msg, &ack_response, sizeof(int));
    // Create Op ID for Send
    send_id = NA_Op_create(
        dtl_handle->mercury_class
    );
    if (send_id == NULL) {
        FLUX_LOG_ERR (dtl_handle->h, "Cannot create Op ID for Mercury Send (ACK: %d)\n", ack_response);
        dyad_ret_code = DYAD_RC_HG_OP_FAIL;
        goto recv_cleanup;
    }
    // Send success notification to module
    ret_code = NA_Msg_send_expected (
        dtl_handle->mercury_class,
        dtl_handle->mercury_ctx,
        dyad_dtl_send_cb,
        (void*) cb_arg,
        send_msg,
        sizeof(int),
        plugin_data,
        *(dtl_handle->remote_addr),
        0,
        dtl_handle->mercury_tag,
        send_id
    );
    if (ret_code != NA_SUCCESS) {
        FLUX_LOG_ERR (dtl_handle->h, "Cannot send response to module through Mercury (ACK: %d)\n", ack_response);
        dyad_ret_code = DYAD_RC_HG_COMM_FAIL;
        goto recv_cleanup;
    }
    rc = wait_on_mercury_op(dtl_handle, cb_arg);
    if (rc != 0) {
        FLUX_LOG_ERR (dtl_handle->h, "Failed to wait for response (NA_Msg_send_expected) to complete (ACK: %d)\n", ack_response);
        // TODO consider calling NA_Cancel here if needed
        dyad_ret_code = DYAD_RC_HG_COMM_FAIL;
        goto recv_cleanup;
    }

recv_cleanup:;
    if (*buf != NULL) {
        free(*buf);
        *buf = NULL;
    }
    if (local_mem_handle != NULL) {
        if (mem_registered) {
            ret_code = NA_Mem_deregister(
                dtl_handle->mercury_class,
                *local_mem_handle
            );
            if (ret_code != NA_SUCCESS)
                FLUX_LOG_ERR (dtl_handle->h, "Could not deregister memory\n");
        }
        NA_Mem_handle_free(
            dtl_handle->mercury_class,
            *local_mem_handle
        );
        local_mem_handle = NULL;
    }
    if (get_id != NULL) {
        NA_Op_destroy(dtl_handle->mercury_class, get_id);
        get_id = NULL;
    }
    if (cb_arg != NULL) {
        free(cb_arg);
    }
    if (send_msg != NULL || plugin_data != NULL) {
        NA_Msg_buf_free(
            dtl_handle->mercury_class,
            send_msg,
            plugin_data
        );
    }
    if (send_id != NULL) {
        NA_Op_destroy(dtl_handle->mercury_class, send_id);
        send_id = NULL;
    }
    return dyad_ret_code;
}

dyad_rc_t dyad_dtl_mercury_close_connection(dyad_dtl_mercury_t *dtl_handle)
{
    dtl_handle->mercury_tag = 0;
    if (dtl_handle->remote_mem_handle != NULL) {
        NA_Mem_handle_free(dtl_handle->mercury_class, *(dtl_handle->remote_mem_handle));
        dtl_handle->remote_mem_handle = NULL;
    }
    if (dtl_handle->remote_addr != NULL) {
        NA_Addr_free(dtl_handle->mercury_class, *(dtl_handle->remote_addr));
        dtl_handle->remote_addr = NULL;
    }
    dtl_handle->data_size = 0;
    return DYAD_RC_OK;
}

dyad_rc_t dyad_dtl_mercury_finalize(dyad_dtl_mercury_t *dtl_handle)
{
    if (dtl_handle != NULL)
    {
        dyad_dtl_mercury_close_connection(dtl_handle);
        // Flux handle should be released by the
        // DYAD context, so it is not released here
        dtl_handle->h = NULL;
        // KVS namespace string should be released by the
        // DYAD context, so it is not released here
        dtl_handle->kvs_namespace = NULL;
        if (dtl_handle->mercury_addr != NULL) {
            NA_Addr_free(dtl_handle->mercury_class, *(dtl_handle->mercury_addr));
            dtl_handle->mercury_addr = NULL;
        }
        if (dtl_handle->mercury_ctx != NULL) {
            NA_Context_destroy(dtl_handle->mercury_class, dtl_handle->mercury_ctx);
            dtl_handle->mercury_ctx = NULL;
        }
        if (dtl_handle->mercury_class) {
            NA_Finalize(dtl_handle->mercury_class);
            dtl_handle->mercury_class = NULL;
        }
        // Free the handle and set to NULL to prevent double free
        free(dtl_handle);
        dtl_handle = NULL;
    }
    return DYAD_RC_OK;
}
