/************************************************************\
 * Copyright 2021 Lawrence Livermore National Security, LLC
 * (c.f. AUTHORS, NOTICE.LLNS, COPYING)
 *
 * This file is part of the Flux resource manager framework.
 * For details, see https://github.com/flux-framework.
 *
 * SPDX-License-Identifier: LGPL-3.0
\************************************************************/

#include "dyad_modules.h"
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
    dyad_dtl_t *dtl_handle;
    dyad_module_get_data_size_t get_size_cb;
    dyad_module_get_t get_cb;
    dyad_module_free_user_data_t free_cb;
    void *user_data;
};

const struct dyad_mod_ctx dyad_mod_ctx_default = {
    NULL,  // h
    false, // debug
    NULL,  // handlers
    NULL,  // dyad_path
    NULL,  // dtl_handle
    NULL,  // get_size_cb
    NULL,  // get_cb
    NULL,  // free_cb
    NULL,  // user_data
};

typedef struct dyad_mod_ctx dyad_mod_ctx_t;

static void dyad_mod_fini (void) __attribute__ ((destructor));

void dyad_mod_fini (void)
{
    flux_t *h = flux_open (NULL, 0);

    if (h != NULL) {
    }
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
    int saved_errno = errno;
    int rc = 0;

    if (!flux_msg_is_streaming (msg)) {
        errno = EPROTO;
        goto fetch_error;
    }

    if (flux_msg_get_userid (msg, &userid) < 0)
        goto fetch_error;

    FLUX_LOG_INFO (h, "DYAD_MOD: unpacking RPC message");

    if (dyad_mod_dtl_rpc_unpack (ctx->dtl_handle, msg, &upath) < 0) {
        FLUX_LOG_ERR (ctx->h, "Could not unpack message from client\n");
        errno = EPROTO;
        goto fetch_error;
    }

    FLUX_LOG_INFO (h, "DYAD_MOD: requested user_path: %s", upath);
    FLUX_LOG_INFO (h, "DYAD_MOD: sending initial response to consumer");

    if (dyad_mod_dtl_rpc_respond (ctx->dtl_handle, msg) < 0) {
        FLUX_LOG_ERR (ctx->h, "Could not send primary RPC response to client\n");
        goto fetch_error;
    }

    strncpy (fullpath, ctx->dyad_path, PATH_MAX - 1);
    concat_str (fullpath, upath, "/", PATH_MAX);

#if DYAD_SPIN_WAIT
    if (!get_stat (fullpath, 1000U, 1000L)) {
        FLUX_LOG_ERR (h, "DYAD_MOD: Failed to access info on \"%s\".\n",
                      fullpath);
        // goto error;
    }
#endif  // DYAD_SPIN_WAIT

    rc = ctx->get_size_cb (fullpath, &inlen, ctx->user_data);
    if (rc < 0) {
        FLUX_LOG_ERR (h, "DYAD_MOD: cannot get size of data to send to consumer\n");
        errno = EIO;
        goto fetch_error;
    }
    rc = ctx->get_cb (fullpath, inbuf, inlen, ctx->user_data);
    if (rc < 0) {
        FLUX_LOG_ERR (h, "DYAD_MOD: cannot read data to send to consumer\n");
        errno = EIO;
        goto fetch_error;
    }

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

static void freectx (void *arg)
{
    int rc = 0;
    dyad_mod_ctx_t *ctx = (dyad_mod_ctx_t *)arg;
    rc = ctx->free_cb(
        &(ctx->user_data)
    );
    if (rc < 0)
        FLUX_LOG_ERR (ctx->h, "Freeing of user data failed!\n");
    flux_msg_handler_delvec (ctx->handlers);
    if (ctx->dtl_handle != NULL) {
        dyad_mod_dtl_finalize (&(ctx->dtl_handle));
        ctx->dtl_handle = NULL;
    }
    free (ctx);
}

dyad_mod_ctx_t *dyad_module_get_ctx (flux_t *h)
{
    dyad_mod_ctx_t *ctx = (dyad_mod_ctx_t *)flux_aux_get (h, "dyad");

    if (!ctx) {
        ctx = (dyad_mod_ctx_t *) malloc (sizeof (*ctx));
        ctx->h = h;
        ctx->debug = false;
        ctx->handlers = NULL;
        ctx->dyad_path = NULL;
        ctx->dtl_handle = NULL;
        ctx->get_cb = NULL;
        ctx->get_size_cb = NULL;
        ctx->user_data = NULL;
        ctx->free_cb = NULL;
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

int dyad_module_open (flux_t *h, dyad_mod_dtl_mode_t dtl_mode, bool debug)
{
    dyad_mod_ctx_t *ctx = getctx (h);
    int rc = 0;
    char *e = NULL;

    ctx->debug = debug;
    rc = dyad_mod_dtl_init (
        dtl_mode,
        h,
        ctx->debug,
        &(ctx->dtl_handle)
    );

    return rc;
}

void dyad_module_set_data_size_get_cb (dyad_mod_ctx_t *ctx,
        dyad_module_get_data_size_t size_getter_cb)
{
    ctx->get_size_cb = size_getter_cb;
}

void dyad_module_set_data_get_cb (dyad_mod_ctx_t *ctx,
        dyad_module_get_t data_getter_cb)
{
    ctx->get_cb = data_getter_cb;
}

void dyad_module_set_user_data (dyad_mod_ctx_t *ctx,
        void *user_data, dyad_module_free_user_data_t free_cb)
{
    ctx->user_data = user_data;
    ctx->free_cb = free_cb;
}

const struct flux_msg_handler_spec dyad_module_callbacks[] = {
    {FLUX_MSGTYPE_REQUEST, "dyad.fetch", dyad_fetch_request_cb, 0},
    FLUX_MSGHANDLER_TABLE_END
};

int dyad_module_run (dyad_mod_ctx_t *ctx) {
    if (flux_msg_handler_addvec (ctx->h, dyad_module_callbacks, (void *)(ctx->h), &ctx->handlers) < 0) {
        FLUX_LOG_ERR (ctx->h, "flux_msg_handler_addvec: %s\n",
                      strerror (errno));
        return EXIT_FAILURE;
    }

    if (flux_reactor_run (flux_get_reactor (ctx->h), 0) < 0) {
        FLUX_LOG_ERR (ctx->h, "flux_reactor_run: %s", strerror (errno));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
