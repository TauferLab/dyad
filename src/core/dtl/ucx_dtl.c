#include "ucx_dtl.h"

#include "dyad_err.h"

#ifdef __cplusplus
#include <cstdlib>
#include <cstdint>
#include <cstdio>
#include <cstring>
#else
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#endif

// Tag mask for UCX Tag send/recv
#define DYAD_UCX_TAG_MASK UINT64_MAX

// Macro function used to simplify checking the status
// of UCX operations
#define UCX_STATUS_FAIL(status) status != UCS_OK

// Define a request struct to be used in handling
// async UCX operations
struct ucx_request {
    int completed;
};
typedef struct ucx_request dyad_ucx_request_t;

// Define a function that UCX will use to allocate and
// initialize our request struct
static void dyad_ucx_request_init(void *request)
{
    dyad_ucx_request_t *real_request = (dyad_ucx_request_t*)request;
    real_request->completed = 0;
}

// Define a function that ucp_tag_msg_recv_nbx will use
// as a callback to signal the completion of the async receive
static void dyad_recv_callback(void *request, ucs_status_t status,
        const ucp_tag_recv_info_t *tag_info, void *user_data)
{
    dyad_ucx_request_t *real_request = (dyad_ucx_request_t*) request;
    real_request->completed = 1;
}

// Simple function used to wait on the async receive
static ucs_status_t dyad_ucx_request_wait(dyad_dtl_ucx_t *dtl_handle, dyad_ucx_request_t *request)
{
    // If 'request' is actually a request handle, this means the communication operation
    // is scheduled, but not yet completed.
    if (UCS_PTR_IS_PTR(request))
    {
        // Spin lock until the request is completed
        // The spin lock shouldn't be costly (performance-wise)
        // because the wait should always come directly after other UCX calls
        // that minimize the size of the worker's event queue.
        // In other words, prior UCX calls should mean that this loop only runs
        // a couple of times at most.
        while (request->completed != 1)
        {
            ucp_worker_progress(dtl_handle->ucx_worker);
        }
        // Get the final status of the communication operation
        ucs_status_t final_request_status = ucp_request_check_status(request);
        // Free and deallocate the request object
        ucp_request_free(request);
        return final_request_status;
    }
    // If 'request' is actually a UCX error, this means the communication
    // operation immediately failed. In that case, we simply grab the 'ucs_status_t'
    // object for the error.
    else if (UCS_PTR_IS_ERR(request))
    {
        return UCS_PTR_STATUS(request);
    }
    // If 'request' is neither a request handle nor an error, then
    // the communication operation immediately completed successfully.
    // So, we simply set the status to UCS_OK
    return UCS_OK;
}

dyad_core_err_t dyad_dtl_ucx_init(flux_t *h, const char *kvs_namespace,
        bool debug, dyad_dtl_ucx_t **dtl_handle)
{
    int retval;

    *dtl_handle = malloc(sizeof(struct dyad_dtl_ucx));
    // Allocation/Freeing of the Flux handle should be
    // handled by the DYAD context
    (*dtl_handle)->h = h;
    // Allocation/Freeing of kvs_namespace should be
    // handled by the DYAD context
    (*dtl_handle)->kvs_namespace = kvs_namespace;
    (*dtl_handle)->ucx_ctx = NULL;
    (*dtl_handle)->ucx_worker = NULL;
    (*dtl_handle)->curr_ep = NULL;

    ucp_params_t ucx_params;
    ucp_worker_params_t worker_params;
    ucp_config_t *config;
    ucs_status_t status;

    // Read the UCX configuration
    status = ucp_config_read (NULL, NULL, &config);
    if (UCX_STATUS_FAIL(status))
    {
        FLUX_LOG_ERR (dtl_handle->h, "Could not read the UCX config\n");
        goto error;
    }

    // Define the settings, parameters, features, etc.
    // for the UCX context. UCX will use this info internally
    // when creating workers, endpoints, etc.
    //
    // The settings enabled are:
    //   * Tag-matching send/recv
    //   * Remote Memory Access communication
    //   * Auto initialization of request objects
    //   * Worker sleep, wakeup, poll, etc. features
    ucx_params.field_mask = UCP_PARAMS_FIELD_FEATURES |
                            UCP_PARAM_FIELD_REQUEST_SIZE |
                            UCP_PARAM_FIELD_REQUEST_INIT;
    ucx_params.features = UCP_FEATURE_TAG |
                          UCP_FEATURE_RMA |
                          UCP_FEATURE_WAKEUP;
    ucx_params.request_size = sizeof(struct ucx_request);
    ucx_params.request_init = dyad_ucx_request_init;

    // Initialize UCX
    status = ucp_init(&ucx_params, config, &(*dtl_handle)->ucx_ctx);

    // If in debug mode, print the configuration of UCX to stderr
    if (debug)
    {
        ucp_config_print(
            config,
            stderr,
            "UCX Configuration",
            UCS_CONFIG_PRINT_CONFIG
        );
    }
    // Release the config
    ucp_config_release(config);
    // Log an error if UCX initialization failed
    if (UCX_STATUS_FAIL(status))
    {
        FLUX_LOG_ERR (h, "ucp_init failed!\n");
        goto error;
    }

    // Define the settings for the UCX worker (i.e., progress engine)
    //
    // The settings enabled are:
    //   * Single-threaded mode (TODO look into multi-threading support)
    //   * Restricting wakeup events to only include Tag-matching recv events
    worker_params.field_mask = UCP_WORKER_PARAM_FIELD_THREAD_MODE |
                               UCP_WORKER_PARAM_FIELD_EVENTS;
    // TODO look into multi-threading support
    worker_params.thread_mode = UCS_THREAD_MODE_SINGLE;
    worker_params.events = UCP_WAKEUP_TAG_RECV;

    // Create the worker and log an error if that fails
    status = ucp_worker_create(
        (*dtl_handle)->ucx_ctx,
        &worker_params,
        &(*dtl_handle)->ucx_worker
    );
    if (UCX_STATUS_FAIL(status))
    {
        FLUX_LOG_ERR (h, "ucp_worker_create failed!\n");
        goto error;
    }

    // Query the worker for its address
    ucp_worker_attr_t worker_attrs;
    worker_attrs.field_mask = UCP_WORKER_ATTR_FIELD_ADDRESS;
    status = ucp_worker_query(
        (*dtl_handle)->ucx_worker,
        &worker_attrs
    );
    if (UCX_STATUS_FAIL(status))
    {
        FLUX_LOG_ERR (h, "Cannot get UCX worker address!\n");
        goto error;
    }
    (*dtl_handle)->consumer_address = worker_attrs.address;
    (*dtl_handle)->addr_len = worker_attrs.address_length;

    goto done;

error:;
    // If an error occured, finalize the DTL handle and
    // return a failing error code
    dyad_dtl_ucx_finalize(*dtl_handle);
    return DYAD_UCXINIT_FAIL;

done:;
    return DYAD_OK;
}

dyad_core_err_t dyad_dtl_ucx_establish_connection(dyad_dtl_ucx_t *dtl_handle,
        uint32_t producer_rank)
{
    // Because we're using tag-matching send/recv for communication,
    // there's no need to do any real connection establishment here.
    // Instead, we use this function to create the tag that will be
    // used for the upcoming communication.
    uint32_t consumer_rank;
    if (flux_get_rank(dtl_handle->h, &consumer_rank) < 0)
    {
        FLUX_LOG_ERR (dtl_handle->h, "Cannot get consumer rank\n");
        return DYAD_FLUXFAIL;
    }
    // The tag is a 64 bit unsigned integer consisting of the
    // 32-bit rank of the producer followed by the 32-bit rank
    // of the consumer
    dtl_handle->comm_tag = (producer_rank << 32) | consumer_rank;
    return DYAD_OK;
}

dyad_core_err_t dyad_dtl_ucx_rpc_pack(dyad_dtl_ucx_t *dtl_handle, const char *upath,
        json_t **packed_obj)
{
    // Use Jansson to pack the tag and UCX address into
    // the payload to be sent via RPC to the producer plugin
    *packed_obj = json_pack(
        "{s:s, s:I, s:s%}",
        "upath",
        upath,
        "tag",
        dtl_handle->comm_tag,
        "ucx_addr",
        dtl_handle->consumer_address, dtl_handle->addr_len
    );
    // If the packing failed, log an error
    if (*packed_obj == NULL)
    {
        FLUX_LOG_ERR (dtl_handle->h, "Could not pack upath and UCX address for RPC\n");
        return DYAD_BADPACK;
    }
    return DYAD_OK;
}

dyad_core_err_t dyad_dtl_ucx_recv(dyad_dtl_ucx_t *dtl_handle, flux_future_t *f,
        void **buf, size_t *buflen)
{
    ucs_status_t status;
    ucp_tag_message_h msg;
    ucp_tag_recv_info_t msg_info;
    // Use 'ucp_worker_wait' to poll the worker until
    // the tag recv event that we're looking for comes in.
    while (true)
    {
        // Probe the tag recv event at the top
        // of the worker's queue
        msg = ucp_tag_probe_nb(
            dtl_handle->ucx_worker,
            dtl_handle->comm_tag,
            DYAD_UCX_TAG_MASK,
            1, // Remove the message from UCP tracking
            // Requires calling ucp_tag_msg_recv_nb
            // with the ucp_tag_message_h to retrieve message
            &msg_info
        );
        // If data has arrived from the producer plugin,
        // break the loop
        if (msg != NULL)
        {
            break;
        }
        // If data has not arrived, check if there are
        // any other events in the worker's queue.
        // If so, start the loop over to handle the next event
        else if (ucp_worker_progress(dtl_handle->ucx_worker))
        {
            continue;
        }
        // No other events are queued. So, we will wait on new
        // events to come in. By using 'ucp_worker_wait' for this,
        // we let the OS do other work in the meantime (no spin locking).
        status = ucp_worker_wait(dtl_handle->ucx_worker);
        // If the wait fails, log an error
        if (UCX_STATUS_FAIL(status))
        {
            FLUX_LOG_ERR (dtl_handle->h, "Could not wait on the message from the producer plugin\n");
            return DYAD_UCXWAIT_FAIL;
        }
    }
    // The metadata retrived from the probed tag recv event contains
    // the size of the data to be sent.
    // So, use that size to allocate a buffer
    *buflen = msg_info.length;
    *buf = malloc(*buflen);
    // If allocation fails, log an error
    if (*buf == NULL)
    {
        FLUX_LOG_ERR (dtl_handle->h, "Could not allocate memory for file\n");
        return DYAD_SYSFAIL;
    }
    // Define the settings for the recv operation
    //
    // The settings enabled are:
    //   * Define callback for the recv because it is async
    //   * Restrict memory buffers to host-only since we aren't directly
    //     dealing with GPU memory
    ucp_request_param_t recv_params;
    // TODO consider enabling UCP_OP_ATTR_FIELD_MEMH to speedup
    // the recv operation if using RMA behind the scenes
    recv_params.op_attr_mask = UCP_OP_ATTR_FIELD_CALLBACK |
                               UCP_OP_ATTR_FIELD_MEMORY_TYPE;
    recv_params.cb.recv = dyad_recv_callback;
    // Constraining to Host memory (as opposed to GPU memory)
    // allows UCX to potentially perform some optimizations
    recv_params.memory_type = UCS_MEMORY_TYPE_HOST;
    // Perform the async recv operation using the probed tag recv event
    dyad_ucx_request_t* req = ucp_tag_msg_recv_nbx(
        dtl_handle->ucx_worker,
        *buf,
        *buflen,
        msg,
        &recv_params
    );
    // Wait on the recv operation to complete
    status = dyad_ucx_request_wait(dtl_handle, req);
    // If the recv operation failed, log an error, free the data buffer,
    // and set the buffer pointer to NULL
    if (UCX_STATUS_FAIL(status))
    {
        FLUX_LOG_ERR (dtl_handle->h, "UCX recv failed!\n");
        free(*buf);
        *buf = NULL;
        return DYAD_UCXCOMM_FAIL;
    }
    return DYAD_OK;
}

dyad_core_err_t dyad_dtl_ucx_close_connection(dyad_dtl_ucx_t *dtl_handle)
{
    // Since we're using tag send/recv, there's no need
    // to explicitly close the connection. So, all we're
    // doing here is setting the tag back to 0 (which cannot
    // be valid for DYAD because DYAD won't send a file from
    // one node to the same node).
    dtl_handle->comm_tag = 0;
    return DYAD_OK;
}

dyad_core_err_t dyad_dtl_ucx_finalize(dyad_dtl_ucx_t *dtl_handle)
{
    if (dtl_handle != NULL)
    {
        // Flux handle should be released by the
        // DYAD context, so it is not released here
        dtl_handle->h = NULL;
        // KVS namespace string should be released by the
        // DYAD context, so it is not released here
        dtl_handle->kvs_namespace = NULL;
        // Release consumer address if not already released
        if (dtl_handle->consumer_address != NULL)
        {
            ucp_worker_release_address(dtl_handle->consumer_address);
            dtl_handle->consumer_address = NULL;
        }
        // Release worker if not already released
        if (dtl_handle->ucx_worker != NULL)
        {
            ucp_worker_destroy(dtl_handle->ucx_worker);
            dtl_handle->ucx_worker = NULL;
        }
        // Release context if not already released
        if (dtl_handle->ucx_ctx != NULL)
        {
            ucp_cleanup(dtl_handle->ucx_ctx);
            dtl_handle->ucx_ctx = NULL;
        }
        // Free the handle and set to NULL to prevent double free
        free(dtl_handle);
        dtl_handle = NULL;
    }
    return DYAD_OK;
}
