/************************************************************\
 * Copyright 2021 Lawrence Livermore National Security, LLC
 * (c.f. AUTHORS, NOTICE.LLNS, COPYING)
 *
 * This file is part of the Flux resource manager framework.
 * For details, see https://github.com/flux-framework.
 *
 * SPDX-License-Identifier: LGPL-3.0
 \************************************************************/

#include "dtl/dyad_mod_dtl.h"
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
#include "dyad_flux_log.h"
#include "dyad_mod_dtl.h"
#include "utils.h"
#include "read_all.h"

#define TIME_DIFF(Tstart, Tend ) \
    ((double) (1000000000L * ((Tend).tv_sec  - (Tstart).tv_sec) + \
        (Tend).tv_nsec - (Tstart).tv_nsec) / 1000000000L)

struct dyad_mod_ctx {
    flux_t *h;
    bool debug;
    flux_msg_handler_t **handlers;
    const char *dyad_path;
    dyad_mod_dtl_t *dtl_handle;
};

typedef struct dyad_mod_ctx dyad_mod_ctx_t;

static void dyad_mod_fini (void) __attribute__((destructor));

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
    if (ctx->dtl_handle != NULL)
    {
        dyad_mod_dtl_finalize(ctx->dtl_handle);
        ctx->dtl_handle = NULL;
    }
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
        ctx->dtl_handle = NULL;
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
    int rc;
    errno = 0;

    if (flux_msg_get_userid (msg, &userid) < 0)
        goto error;

    // UCX_TODO Tweak unpack to retrieve consumer UCP address
    rc = dyad_mod_dtl_rpc_unpack(ctx->dtl_handle, msg, &upath);
    if (rc < 0)
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
        close (fd);
        goto error;
    }
    close (fd);

    // UCX_TODO add code to send file to consumer
    rc = dyad_mod_dtl_establish_connection(ctx->dtl_handle);
    if (rc < 0)
    {
        // TODO log error
        goto error;
    }
    rc = dyad_mod_dtl_send(ctx->dtl_handle, inbuf, inlen);
    if (rc == 1)
    {
        DYAD_LOG_ERR(ctx, "Connection was not correctly established before send was called\n");
        goto error;
    }
    else if (rc < 0)
    {
        // TODO log error
        goto error;
    }
    rc = dyad_mod_dtl_close_connection(ctx->dtl_handle);

error:
    if (flux_respond_error (h, msg, errno, NULL) < 0) {
        FLUX_LOG_ERR (h, "DYAD_MOD: %s: flux_respond_error", __FUNCTION__);
    }
    return;

done:
    errno = saved_errno;
    return;
}

static int dyad_open (flux_t *h, dyad_mod_dtl_mode_t dtl_mode)
{
    dyad_mod_ctx_t *ctx = getctx (h);
    int rc = -1;
    char *e = NULL;

    if ((e = getenv ("DYAD_MOD_DEBUG")) && atoi (e))
        ctx->debug = true;
    rc = dyad_mod_dtl_init(
        dtl_mode,
        h,
        ctx->debug,
        &(ctx->dtl_handle)
    );

    return rc;
}

static const struct flux_msg_handler_spec htab[] = {
    { FLUX_MSGTYPE_REQUEST, "dyad.fetch",  dyad_fetch_request_cb, 0 },
    FLUX_MSGHANDLER_TABLE_END
};

void usage()
{
    // TODO make absolutely sure this type of string
    // literal concatenation works in C90.
    fprintf(stderr,
            "Usage: flux module load dyad.so <MANAGED_PATH> <DTL_MODE>\n\n"
            "Arguments:\n==========\n"
            "  * MANAGED_PATH: path for DYAD to manage\n"
            "  * DTL_MODE: mode for data movement. Can be one of:\n"
            "    - FLUX: use Flux RPC for data movement\n"
            "    - UCX: use UCX for data movement\n");
}

int mod_main (flux_t *h, int argc, char **argv)
{
    int final_retval; // Final return value
    char *kvs_key = NULL; // Flux KVS key to store the module's UCP address under
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
    if (argc != 2) {
        DYAD_LOG_ERR (ctx, "DYAD_MOD: Missing argument. " \
                "Requires a local dyad path and a DTL mode specified.\n");
        usage();
        goto error;
    }
    (ctx->dyad_path) = argv[0]; // First (and only) argument is the managed path
    size_t mode_str_len = strlen(argv[1]);
    dyad_mod_dtl_mode_t dtl_mode;
    if (strncmp(argv[1], "FLUX", mode_str_len <= 4 ? mode_str_len : 4))
    {
        dtl_mode = DYAD_DTL_FLUX_RPC;
    }
    else if (strncmp(argv[1], "UCX", mode_str_len <= 3 ? mode_str_len : 3))
    {
        dtl_mode = DYAD_DTL_UCX;
    }
    else
    {
        DYAD_LOG_ERR (ctx, "DYAD_MOD: invalid DTL mode (%s)\n", argv[1]);
        goto error;
    }
    mkdir_as_needed (ctx->dyad_path, m); // Create the managed path if it doesn't exist

    if (dyad_open (h, dtl_mode) < 0) {
        DYAD_LOG_ERR (ctx, "dyad_open failed");
        goto error;
    }

    // Start the Plugin
    fprintf (stderr, "dyad module begins using \"%s\"\n", argv[0]);
    DYAD_LOG_INFO (ctx, "dyad module begins using \"%s\"\n", argv[0]);

    if (flux_msg_handler_addvec (ctx->h, htab, (void *)h,
                &ctx->handlers) < 0) {
        DYAD_LOG_ERR (ctx, "flux_msg_handler_addvec: %s\n", strerror (errno));
        goto error;
    }

    if (flux_reactor_run (flux_get_reactor (ctx->h), 0) < 0) {
        DYAD_LOG_ERR (ctx, "flux_reactor_run: %s", strerror (errno));
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
    return final_retval;
}

MOD_NAME ("dyad");

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
