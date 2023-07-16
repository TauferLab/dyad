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
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#else
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#endif  // defined(__cplusplus)

#include <fcntl.h>
#include <linux/limits.h>
#include <sys/types.h>
#include <unistd.h>

#include "dtl/dyad_mod_dtl.h"
#include "storage_check.h"
#include "read_all.h"
#include "utils.h"

#define TIME_DIFF(Tstart, Tend)                                                \
    ((double)(1000000000L * ((Tend).tv_sec - (Tstart).tv_sec) + (Tend).tv_nsec \
              - (Tstart).tv_nsec)                                              \
     / 1000000000L)

struct dyad_mod_ctx {
    flux_t *h;
    bool debug;
    flux_msg_handler_t **handlers;
    const char *dyad_path;
    dyad_mod_dtl_t *dtl_handle;
    storage_view_t *view;
};

const struct dyad_mod_ctx dyad_mod_ctx_default = {
    NULL,
    false,
    NULL,
    NULL,
    NULL,
    NULL
};

typedef struct dyad_mod_ctx dyad_mod_ctx_t;

static void dyad_mod_fini (void) __attribute__ ((destructor));

void dyad_mod_fini (void)
{
    flux_t *h = flux_open (NULL, 0);

    if (h != NULL) {
    }
}

static void freectx (void *arg)
{
    dyad_mod_ctx_t *ctx = (dyad_mod_ctx_t *)arg;
    flux_msg_handler_delvec (ctx->handlers);
    if (ctx->dtl_handle != NULL) {
        dyad_mod_dtl_finalize (&(ctx->dtl_handle));
        ctx->dtl_handle = NULL;
    }
    if (ctx->view != NULL) {
        finalize_storage_view (&(ctx->view));
        ctx->view = NULL;
    }
    free (ctx);
}

static dyad_mod_ctx_t *getctx (flux_t *h)
{
    dyad_mod_ctx_t *ctx = (dyad_mod_ctx_t *)flux_aux_get (h, "dyad");

    if (!ctx) {
        ctx = (dyad_mod_ctx_t *) malloc (sizeof (*ctx));
        ctx->h = h;
        ctx->debug = false;
        ctx->handlers = NULL;
        ctx->dyad_path = NULL;
        ctx->dtl_handle = NULL;
        ctx->view = NULL;
        if (flux_aux_set (h, "dyad", ctx, freectx) < 0) {
            FLUX_LOG_ERR (h, "DYAD_MOD: flux_aux_set() failed!\n");
            goto getctx_error;
        }
    }

    goto getctx_done;

getctx_error:;
    return NULL;

getctx_done:
    return ctx;
}

/* request callback called when dyad.fetch request is invoked */
#if DYAD_PERFFLOW
__attribute__ ((annotate ("@critical_path()")))
#endif
static void dyad_fetch_request_cb (flux_t *h,
    flux_msg_handler_t *w,
    const flux_msg_t *msg,
    void *arg)
{
    FLUX_LOG_INFO (h, "Launched callback for dyad.fetch\n");
    dyad_mod_ctx_t *ctx = getctx (h);
    ssize_t inlen = 0;
    void *inbuf = NULL;
    int fd = -1;
    uint32_t userid = 0u;
    char *upath = NULL;
    char fullpath[PATH_MAX + 1] = {'\0'};
    json_t *consumer_storage_record = NULL;
    storage_entry_t *consumer_storage_entry = NULL;
    storage_check_record_t *producer_storage_record = NULL;
    bool should_transfer = false;
    bool is_local_cons = false;
    int saved_errno = errno;
    int rc = 0;

    if (!flux_msg_is_streaming (msg)) {
        errno = EPROTO;
        goto fetch_error;
    }

    if (flux_msg_get_userid (msg, &userid) < 0)
        goto fetch_error;

    FLUX_LOG_INFO (h, "DYAD_MOD: unpacking RPC message");

    if (dyad_mod_dtl_rpc_unpack (ctx->dtl_handle, msg, &upath,
            &consumer_storage_record, &is_local_cons) < 0) {
        FLUX_LOG_ERR (ctx->h, "Could not unpack message from client\n");
        errno = EPROTO;
        goto fetch_error;
    }

    FLUX_LOG_INFO (h, "DYAD_MOD: requested user_path: %s", upath);

    strncpy (fullpath, ctx->dyad_path, PATH_MAX - 1);
    concat_str (fullpath, upath, "/", PATH_MAX);
    
    if (!is_local_cons) {
        consumer_storage_entry = unpack_storage_entry (consumer_storage_record);
        if (consumer_storage_entry == NULL) {
            FLUX_LOG_ERR (h, "DYAD_MOD: failed to unpack storage entry from consumer\n");
            errno = EINVAL;
            goto fetch_error;
        }
        producer_storage_record = check_if_local_storage (ctx->view, fullpath);
        if (producer_storage_record == NULL) {
            FLUX_LOG_ERR (h, "DYAD_MOD: failed to check for producer storage device\n");
            errno = EINVAL;
            goto fetch_error;
        }
        if (check_storage_entry_eq (consumer_storage_entry, producer_storage_record->storage_entry_ptr)) {
            should_transfer = false;
        } else {
            should_transfer = true;
        }
        free_storage_entry (&consumer_storage_entry);
        free_storage_check_record (&producer_storage_record);
    } else {
        should_transfer = true;
    }
    
    rc = flux_respond_pack (
        h,
        msg,
        "b",
        should_transfer
    );
    if (rc != 0) {
        FLUX_LOG_ERR (h, "Could not send response for shared storage to consumer\n");
        goto fetch_error;
    }
    if (!should_transfer) {
        goto fetch_success;
    }

    FLUX_LOG_INFO (h, "DYAD_MOD: sending initial response to consumer");
    if (dyad_mod_dtl_rpc_respond (ctx->dtl_handle, msg) < 0) {
        FLUX_LOG_ERR (ctx->h, "Could not send primary RPC response to client\n");
        goto fetch_error;
    }

#if DYAD_SPIN_WAIT
    if (!get_stat (fullpath, 1000U, 1000L)) {
        FLUX_LOG_ERR (h, "DYAD_MOD: Failed to access info on \"%s\".\n",
                      fullpath);
        // goto error;
    }
#endif  // DYAD_SPIN_WAIT

    FLUX_LOG_INFO (h, "Reading file %s for transfer", fullpath);
    fd = open (fullpath, O_RDONLY);
    if (fd < 0) {
        FLUX_LOG_ERR (h, "DYAD_MOD: Failed to open file \"%s\".\n", fullpath);
        goto fetch_error;
    }
    if ((inlen = read_all (fd, &inbuf)) < 0) {
        FLUX_LOG_ERR (h, "DYAD_MOD: Failed to load file \"%s\".\n", fullpath);
        goto fetch_error;
    }
    close (fd);
    FLUX_LOG_INFO (h, "Is inbuf NULL? -> %i\n", (int) (inbuf == NULL));

    FLUX_LOG_INFO (h, "Establish DTL connection with consumer");
    if (dyad_mod_dtl_establish_connection (ctx->dtl_handle) < 0) {
        FLUX_LOG_ERR (ctx->h, "Could not establish DTL connection with client\n");
        errno = ECONNREFUSED;
        goto fetch_error;
    }
    FLUX_LOG_INFO (h, "Send file to consumer with DTL");
    rc = dyad_mod_dtl_send (ctx->dtl_handle, inbuf, inlen);
    FLUX_LOG_INFO (h, "Close DTL connection with consumer");
    dyad_mod_dtl_close_connection (ctx->dtl_handle);
    free(inbuf);
    if (rc < 0) {
        FLUX_LOG_ERR (ctx->h, "Could not send data to client via DTL\n");
        errno = ECOMM;
        goto fetch_error;
    }

fetch_success:
    FLUX_LOG_INFO (h, "Close RPC message stream with an ENODATA message");
    if (flux_respond_error (h, msg, ENODATA, NULL) < 0) {
        FLUX_LOG_ERR (h, "DYAD_MOD: %s: flux_respond_error with ENODATA failed\n", __FUNCTION__);
    }
    errno = saved_errno;
    FLUX_LOG_INFO (h, "Finished dyad.fetch module invocation\n");
    return;

fetch_error:
    FLUX_LOG_INFO (h, "Close RPC message stream with an error (errno = %d)\n", errno);
    if (flux_respond_error (h, msg, errno, NULL) < 0) {
        FLUX_LOG_ERR (h, "DYAD_MOD: %s: flux_respond_error", __FUNCTION__);
    }
    errno = saved_errno;
    return;
}

static int dyad_open (flux_t *h, dyad_mod_dtl_mode_t dtl_mode, bool debug)
{
    dyad_mod_ctx_t *ctx = getctx (h);
    int rc = 0;
    char *e = NULL;

    ctx->debug = debug;
    // TODO add config setting to define storage graph ingestion
    //      method once more methods are added
    ctx->view = init_storage_view (LINUX_PROC_MOUNTS);
    if (ctx->view == NULL) {
        FLUX_LOG_ERR (h, "Could not create local view of storage graph");
        return -1;
    }
    rc = dyad_mod_dtl_init (
        dtl_mode,
        h,
        ctx->debug,
        &(ctx->dtl_handle)
    );

    return rc;
}

static const struct flux_msg_handler_spec htab[] = {{FLUX_MSGTYPE_REQUEST,
                                                     "dyad.fetch",
                                                     dyad_fetch_request_cb, 0},
                                                    FLUX_MSGHANDLER_TABLE_END};

void usage()
{
    fprintf(stderr, "Usage: flux exec -r all flux module load dyad.so <PRODUCER_PATH> [DTL_MODE] [--debug | -d]\n\n");
    fprintf(stderr, "Required Arguments:\n");
    fprintf(stderr, "===================\n");
    fprintf(stderr, " * PRODUCER_PATH: the producer-managed path that the module should track\n\n");
    fprintf(stderr, "Optional Arguments:\n");
    fprintf(stderr, "===================\n");
    fprintf(stderr, " * DTL_MODE: a valid data transport layer (DTL) mode. Can be one of the following values\n");
    fprintf(stderr, "   * UCX (default): use UCX to send data to consumer\n");
    fprintf(stderr, "   * FLUX_RPC: use Flux's RPC response mechanism to send data to consumer\n");
    fprintf(stderr, " * --debug | -d: if provided, add debugging log messages\n");
}

int mod_main (flux_t *h, int argc, char **argv)
{
    const mode_t m = (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH | S_ISGID);
    dyad_mod_ctx_t *ctx = NULL;
    size_t flag_len = 0;
    dyad_mod_dtl_mode_t dtl_mode = DYAD_DTL_UCX;
    bool debug = false;

    if (!h) {
        fprintf (stderr, "Failed to get flux handle\n");
        goto mod_done;
    }

    ctx = getctx (h);

    if (argc < 1) {
        FLUX_LOG_ERR (ctx->h,
                      "DYAD_MOD: Missing argument(s). "
                      "Requires a local dyad path.\n");
        usage();
        goto mod_error;
    }
    (ctx->dyad_path) = argv[0];
    mkdir_as_needed (ctx->dyad_path, m);

    if (argc >= 2) {
        FLUX_LOG_INFO (h, "DTL Mode (from cmd line) is %s\n", argv[1]);
        flag_len = strlen(argv[1]);
        if (strncmp (argv[1], "FLUX_RPC", flag_len) == 0) {
            dtl_mode = DYAD_DTL_FLUX_RPC;
        } else if (strncmp (argv[1], "UCX", flag_len) == 0) {
            dtl_mode = DYAD_DTL_UCX;
        } else {
            FLUX_LOG_ERR (ctx->h, "Invalid DTL mode provided\n");
            usage();
            goto mod_error;
        }
    }

    if (argc >= 3) {
        flag_len = strlen (argv[2]);
        if (strncmp (argv[2], "--debug", flag_len) == 0 || strncmp (argv[2], "-d", flag_len) == 0) {
            debug = true;
        } else {
            debug = false;
        }
    }

    if (dyad_open (h, dtl_mode, debug) < 0) {
        FLUX_LOG_ERR (ctx->h, "dyad_open failed");
        goto mod_error;
    }

    FLUX_LOG_INFO (ctx->h, "dyad module begins using \"%s\"\n", argv[0]);

    if (flux_msg_handler_addvec (ctx->h, htab, (void *)h, &ctx->handlers) < 0) {
        FLUX_LOG_ERR (ctx->h, "flux_msg_handler_addvec: %s\n",
                      strerror (errno));
        goto mod_error;
    }

    if (flux_reactor_run (flux_get_reactor (ctx->h), 0) < 0) {
        FLUX_LOG_ERR (ctx->h, "flux_reactor_run: %s", strerror (errno));
        goto mod_error;
    }

    goto mod_done;

mod_error:;
    return EXIT_FAILURE;

mod_done:;
    return EXIT_SUCCESS;
}

MOD_NAME ("dyad");

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
