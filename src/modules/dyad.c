/************************************************************\
 * Copyright 2021 Lawrence Livermore National Security, LLC
 * (c.f. AUTHORS, NOTICE.LLNS, COPYING)
 *
 * This file is part of the Flux resource manager framework.
 * For details, see https://github.com/flux-framework.
 *
 * SPDX-License-Identifier: LGPL-3.0
 \************************************************************/

#if defined(__cplusplus)
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <ctime>
#include <cstddef>
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stddef.h>
#endif // defined(__cplusplus)

#include <linux/limits.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <flux/core.h>
#include "dyad_ctx.h"
#include "utils.h"
#include "read_all.h"

#define TIME_DIFF(Tstart, Tend ) \
    ((double) (1000000000L * ((Tend).tv_sec  - (Tstart).tv_sec) + \
        (Tend).tv_nsec - (Tstart).tv_nsec) / 1000000000L)

#if !defined(DYAD_LOGGING_ON) || (DYAD_LOGGING_ON == 0)
#define FLUX_LOG_INFO(...) do {} while (0)
#define FLUX_LOG_ERR(...) do {} while (0)
#else
#define FLUX_LOG_INFO(h, ...) flux_log (h, LOG_INFO, __VA_ARGS__)
#define FLUX_LOG_ERR(h, ...) flux_log_error (h, __VA_ARGS__)
#endif

#define DYAD_UCP_TAG_MASK UINT32_MAX // Use UINT32_MAX as the mask to reserve
                                     // the second 32 bits of ucp_tag_t (uint64_t)
                                     // for future use

struct ucx_request {
    bool completed;
};
typedef struct ucx_request dyad_ucx_request_t;

static void dyad_ucx_request_init(void *request)
{
    dyad_ucx_request_t *real_request = (dyad_ucx_request_t*)request;
    real_request->completed = false;
}

static ucs_status_t curr_client_status;

static void dyad_ep_err_handler (void *arg, ucp_ep_h ep, ucs_status_t status)
{
    ucs_status_t *curr_status_ptr = (ucs_status_t*) arg;
    FLUX_LOG_ERR ();
}

static void dyad_mod_fini (void) __attribute__((destructor));

void dyad_mod_fini (void)
{
    flux_t *h = flux_open (NULL, 0);

    if (h != NULL) {
    }
}

static void ucx_finalize (dyad_mod_ctx_t *ctx)
{
    // Destroy the UCP worker (i.e., progress engine)
    // This should always execute, but guard with a conditional to be safe
    if (ctx->ucx_worker != NULL)
    {
        ucp_worker_destroy(ctx->ucx_worker);
        ctx->ucx_worker = NULL;
    }
    // Destroy the UCP context
    // This should always execute, but guard with a conditional to be safe
    if (ctx->ucx_ctx != NULL)
    {
        ucp_cleanup(ctx->ucx_ctx);
        ctx->ucx_ctx = NULL;
    }
}

static void freectx (void *arg)
{
    dyad_mod_ctx_t *ctx = (dyad_mod_ctx_t *)arg;
    flux_msg_handler_delvec (ctx->handlers);
    ucx_finalize (ctx);
    free (ctx);
}

static dyad_mod_ctx_t *getctx (flux_t *h)
{
    dyad_mod_ctx_t *ctx = (dyad_mod_ctx_t *) flux_aux_get (h, "dyad");

    if (!ctx) {
        ctx = (dyad_mod_ctx_t *) malloc (sizeof (*ctx));
        ctx->h = h;
        ctx->debug = false;
        ctx->handlers = NULL;
        ctx->dyad_path = NULL;
        ctx->ucx_ctx = NULL;
        ctx->ucx_worker = NULL;
        if (flux_aux_set (h, "dyad", ctx, freectx) < 0) {
            FLUX_LOG_ERR (h, "DYAD_MOD: flux_aux_set() failed!\n");
            goto error;
        }
    }

    goto done;

error:;
      return NULL;

done:
      return ctx;
}

#if DYAD_PERFFLOW
__attribute__((annotate("@critical_path()")))
#endif
static int dyad_establish_ucx_connection (dyad_mod_ctx_t *ctx, ucp_address_t *cons_addr,
        ucp_tag_t tag, ucp_ep_h *ep)
{
    ucp_ep_params_t ep_params;
    ep_params.field_mask = UCP_EP_PARAM_FIELD_REMOTE_ADDRESS |
        UCP_EP_PARAM_FIELD_ERR_HANDLING_MODE |
        UCP_EP_PARAM_FIELD_ERR_HANDLER |
        UCP_EP_PARAM_FIELD_USER_DATA;
    ep_params.address = cons_addr;
    ep_params.err_mode = UCP_ERR_HANDLING_MODE_NONE; // Currently disabling UCX error reporting,
                                                     // but can enable later if desired
    ep_params.err_handler.cb = ;
    ep_params.err_handler.arg = NULL;
}

#if DYAD_PERFFLOW
__attribute__((annotate("@critical_path()")))
#endif
static int dyad_data_size_send (dyad_mod_ctx_t *ctx, ssize_t data_size)
{
}

#if DYAD_PERFFLOW
__attribute__((annotate("@critical_path()")))
#endif
static int dyad_send_data (dyad_mod_ctx_t *ctx, void *buf, ssize_t buflen)
{
}

/* request callback called when dyad.fetch request is invoked */
#if DYAD_PERFFLOW
__attribute__((annotate("@critical_path()")))
#endif
static void dyad_fetch_request_cb (flux_t *h, flux_msg_handler_t *w,
        const flux_msg_t *msg, void *arg)
{
    dyad_mod_ctx_t *ctx = getctx (h);
    ssize_t inlen = 0;
    void *inbuf = NULL;
    int fd = -1;
    uint32_t userid = 0u;
    char *upath = NULL;
    char fullpath[PATH_MAX+1] = {'\0'};
    int saved_errno = errno;
    errno = 0;

    if (flux_msg_get_userid (msg, &userid) < 0)
        goto error;

    // UCX_TODO Tweak unpack to retrieve consumer UCP address
    if (flux_request_unpack (msg, NULL, "{s:s}", "upath", &upath) < 0)
        goto error;

    FLUX_LOG_INFO (h, "DYAD_MOD: requested user_path: %s", upath);

    strncpy (fullpath, ctx->dyad_path, PATH_MAX-1);
    concat_str (fullpath, upath, "/", PATH_MAX);

#if DYAD_SPIN_WAIT
    if (!get_stat (fullpath, 1000U, 1000L)) {
        FLUX_LOG_ERR (h, "DYAD_MOD: Failed to access info on \"%s\".\n", \
                fullpath);
        //goto error;
    }
#endif // DYAD_SPIN_WAIT

    fd = open (fullpath, O_RDONLY);
    if (fd < 0) {
        FLUX_LOG_ERR (h, "DYAD_MOD: Failed to open file \"%s\".\n", fullpath);
        goto error;
    }
    if ((inlen = read_all (fd, &inbuf)) < 0) {
        FLUX_LOG_ERR (h, "DYAD_MOD: Failed to load file \"%s\".\n", fullpath);
        goto error;
    }
    close (fd);

    // UCX_TODO add code to send file to consumer

error:
    if (flux_respond_error (h, msg, errno, NULL) < 0) {
        FLUX_LOG_ERR (h, "DYAD_MOD: %s: flux_respond_error", __FUNCTION__);
    }
    return;

done:
    errno = saved_errno;
    return;
}

static int dyad_open (flux_t *h)
{
    dyad_mod_ctx_t *ctx = getctx (h);
    int rc = -1;
    char *e = NULL;

    if ((e = getenv ("DYAD_MOD_DEBUG")) && atoi (e))
        ctx->debug = true;
    rc = 0;

    return rc;
}

static const struct flux_msg_handler_spec htab[] = {
    { FLUX_MSGTYPE_REQUEST, "dyad.fetch",  dyad_fetch_request_cb, 0 },
    FLUX_MSGHANDLER_TABLE_END
};

int mod_main (flux_t *h, int argc, char **argv)
{
    int final_retval; // Final return value
    char *kvs_key = NULL; // Flux KVS key to store the module's UCP address under
    ucp_address_t *plugin_address = NULL; // Module's UCP address
    const mode_t m = (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH | S_ISGID); // Access modes for reading the file
    dyad_mod_ctx_t *ctx = NULL; // DYAD Module Context

    // Need Flux handle, so ensure we've got one
    if (!h) {
        fprintf (stderr, "Failed to get flux handle\n");
        goto done;
    }

    // Get and/or create DYAD Module Context using the Flux handle
    // Also, cache the DYAD Module Context if it is created
    ctx = getctx (h);

    // Parse command-line arguments
    if (argc != 1) {
        FLUX_LOG_ERR (ctx->h, "DYAD_MOD: Missing argument. " \
                "Requires a local dyad path specified.\n");
        fprintf  (stderr,
                "Missing argument. Requires a local dyad path specified.\n");
        goto error;
    }
    (ctx->dyad_path) = argv[0]; // First (and only) argument is the managed path
    mkdir_as_needed (ctx->dyad_path, m); // Create the managed path if it doesn't exist

    if (dyad_open (h) < 0) {
        FLUX_LOG_ERR (ctx->h, "dyad_open failed");
        goto error;
    }

    // Create and startup UCX context and worker
    ucp_params_t ucx_params; // parameters for UCP Context creation
    ucp_worker_params_t worker_params; // parameters for UCP Worker creation
    ucp_config_t *config; // UCP configuration info
    ucs_status_t status; // variable to store the return codes/statuses of
                         // UCP functions

                         // Load UCP config
    status = ucp_config_read (NULL, NULL, &config);
    // If config loading failed, raise an error
    if (status != UCS_OK)
    {
        FLUX_LOG_ERR (ctx->h, "Could not read UCP config\n");
        goto error;
    }

    // Enable the following parameters for UCP Context creation
    //   * "features" (UCP_PARAMS_FIELD_FEATURES)
    //   * "request_size" (UCP_PARAM_FIELD_REQUEST_SIZE)
    //   * "request_init" (UCP_PARAM_FIELD_REQUEST_INIT)
    ucx_params.field_mask = UCP_PARAMS_FIELD_FEATURES |
        UCP_PARAM_FIELD_REQUEST_SIZE |
        UCP_PARAM_FIELD_REQUEST_INIT;
    // Enable the following UCP features
    //   * Tag Matching Send/Recv
    //   * RMA (in case it's needed for Rendezvous protocol)
    //   * Wakeup (enables efficient blocking for async communication)
    ucx_params.features = UCP_FEATURE_TAG |
        UCP_FEATURE_RMA |
        UCP_FEATURE_WAKEUP;
    // Define the size of the request objects returned by async UCP
    // communication routines
    ucx_params.request_size = sizeof(struct ucx_request);
    // Define the initializer function for request objects
    ucx_params.request_init = dyad_ucx_request_init;
    // Create the UCP Context
    status = ucp_init(&ucx_params, config, &ctx->ucx_ctx);

    // If running in debug mode, print out the configuration
    if (ctx->debug)
    {
        ucp_config_print(
                config,
                stderr,
                "UCX Configuration",
                UCS_CONFIG_PRINT_CONFIG
                );
    }
    // Config is no longer needed, so release it
    ucp_config_release(config);
    // If Context creation failed, raise an error
    if (status != UCS_OK)
    {
        FLUX_LOG_ERR (ctx->h, "ucp_init failed\n");
        goto error;
    }

    // Enable the following UCP Worker features
    //   * thread_mode (UCP_WORKER_PARAM_FIELD_THREAD_MODE)
    worker_params.field_mask = UCP_WORKER_PARAM_FIELD_THREAD_MODE;
    // Set the thread mode to "single"
    // TODO make absolutely sure that there's no risk of multithreading
    // within the DYAD plugin
    worker_params.thread_mode = UCS_THREAD_MODE_SINGLE;
    // Create the UCP worker
    status = ucp_worker_create(ctx->ucx_ctx, &worker_params, &ctx->ucx_worker);
    // If Worker creation failed, raise an error
    if (status != UCS_OK)
    {
        FLUX_LOG_ERR (ctx->h, "ucp_worker_create failed\n");
        goto error;
    }

    ucp_worker_attr_t worker_attrs; // UCP Worker attributes
                                    // Stores the address and address length
                                    // after calling ucp_worker_query
                                    // Query the UCP Worker for it's address
    worker_attrs.field_mask = UCP_WORKER_ATTR_FIELD_ADDRESS;
    // Get the address of the UCP worker
    status = ucp_worker_query(
            ctx->ucx_worker,
            &worker_attrs
            );
    // If getting the Worker address failed, raise an error
    if (status != UCS_OK)
    {
        FLUX_LOG_ERR (ctx->h, "Cannot get the UCX worker address\n");
        goto error;
    }
    plugin_address = worker_attrs.address;

    // Build a key to use in storing the Worker address in the KVS
    uint32_t rank;
    flux_kvs_txn_t *txn;

    // Get the Flux rank, or error on fail
    if (flux_get_rank(ctx->h, &rank) < 0)
    {
        FLUX_LOG_ERR (ctx->h, "Cannot get Flux rank\n");
        goto error;
    }

    // Convert the rank to a string
    char rank_str[11]; // Max value of uint32_t is 10 digits
    int num_chars = snprintf(rank_str, 11, "%lu", rank);
    // Raise an error if the rank cannot be converted to a string
    if (num_chars <= 0)
    {
        FLUX_LOG_ERR (ctx->h, "Cannot build KVS key for UCX info\n");
        goto error;
    }
    // Create the KVS key as "dyad_addr_<RANK_STRING>"
    kvs_key = malloc(11 + strlen(rank_str)); // 11 for "dyad_addr_" and the NUL terminator
    num_chars = snprintf(kvs_key, 11+strlen(rank_str), "dyad_addr_%s", rank_str);
    // Raise an error if the KVS key cannot be created
    // 10 because return value of snprintf does not include NUL terminator
    if (num_chars != 10+strlen(rank_str))
    {
        FLUX_LOG_ERR (ctx->h, "Cannot build KVS key for UCX info\n");
        goto error;
    }

    // Create a Flux TXN object for saving the UCP address to the KVS
    // If creation fails, raise an error
    if (!(txn = flux_kvs_txn_create()))
    {
        FLUX_LOG_ERR (ctx->h, "Failed to create flux_kvs_txn for UCX\n");
        goto error;
    }
    // Pack the address and its size into the TXN
    // If the packing fails, raise an error
    // TODO see if we can pass length as a size_t instead of an int
    if (flux_kvs_txn_put_raw(txn, 0, kvs_key, plugin_address, (int) worker_attrs->address_length) < 0)
    {
        FLUX_LOG_ERR (ctx->h, "Failed to pack UCP address into flux_kvs_txn\n");
        goto error;
    }

    flux_future_t *f = NULL; // Future produced when committing the transaction
                             // Get the KVS namespace from the same environment variable that the
                             // Producers/Consumers use
                             // TODO try to find another way to pass this
    char *kvs_namespace = NULL;
    // If getenv fails, log an error and exit
    if (!(kvs_namespace = getenv ("FLUX_KVS_NAMESPACE")))
    {
        FLUX_LOG_ERR (ctx->h, "Could not determine KVS namespace\n");
        goto error;
    }

    // Commit the transaction to the Flux KVS
    // If the commit fails, log an error and exit
    if (!(f = flux_kvs_commit(ctx->h, kvs_namespace, 0, txn)))
    {
        FLUX_LOG_ERR (ctx->h, "Could not commit transaction to Flux KVS\n");
        goto error;
    }

    // Wait for the Future to be resolved
    flux_future_wait_for(f, -1.0);

    // Destroy/Deallocate the Future, Transaction (TXN), key buffer, and UCP address
    flux_future_destroy(f);
    flux_kvs_txn_destroy(txn);
    free(kvs_key);
    ucp_worker_release_address(ctx->ucx_worker, plugin_address);
    kvs_key = NULL;
    plugin_address = NULL;

    // Start the Plugin
    fprintf (stderr, "dyad module begins using \"%s\"\n", argv[0]);
    FLUX_LOG_INFO (ctx->h, "dyad module begins using \"%s\"\n", argv[0]);

    if (flux_msg_handler_addvec (ctx->h, htab, (void *)h,
                &ctx->handlers) < 0) {
        FLUX_LOG_ERR (ctx->h, "flux_msg_handler_addvec: %s\n", strerror (errno));
        goto error;
    }

    if (flux_reactor_run (flux_get_reactor (ctx->h), 0) < 0) {
        FLUX_LOG_ERR (ctx->h, "flux_reactor_run: %s", strerror (errno));
        goto error;
    }

    goto done;

    // If an error occurs, set the return value to indicate an error
    // and goto the cleanup step
error:;
    final_retval = EXIT_FAILURE;
    goto cleanup;

    // If no error occurs, set the return value to indicate success
    // and goto the cleanup step
done:;
     final_retval = EXIT_SUCCESS;

cleanup:;
    // Free the KVS key if not already done
    if (kvs_key != NULL)
    {
        free(kvs_key);
        kvs_key = NULL;
    }
    // Release the plugin UCP address if not already done
    if (plugin_address != NULL)
    {
        ucp_worker_release_address(ctx->ucx_worker, plugin_address);
        plugin_address = NULL;
    }
    ucx_finalize(ctx);
    return final_retval;
}

MOD_NAME ("dyad");

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
