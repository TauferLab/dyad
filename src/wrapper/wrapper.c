/************************************************************\
 * Copyright 2021 Lawrence Livermore National Security, LLC
 * (c.f. AUTHORS, NOTICE.LLNS, COPYING)
 *
 * This file is part of the Flux resource manager framework.
 * For details, see https://github.com/flux-framework.
 *
 * SPDX-License-Identifier: LGPL-3.0
\************************************************************/

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif // _GNU_SOURCE

#if defined(__cplusplus)
#include <cstdio>
#include <cstdlib>
#include <climits>
#include <cstring>
#include <cerrno>
#include <cstdarg>
#include <ctime>
using namespace std; // std::clock ()
//#include <cstdbool> // c++11
#else
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <time.h>
#include <stdbool.h>
#endif // defined(__cplusplus)

#include <fcntl.h>
#include <libgen.h> // dirname
#include <dlfcn.h>
#include <unistd.h>
#include "utils.h"
#include "wrapper.h"
#include "dyad_core.h"
#include "dyad_err.h"

__thread dyad_ctx_t *ctx = NULL;
static void dyad_sync_init (void) __attribute__((constructor));
static void dyad_sync_fini (void) __attribute__((destructor));

int open_real (const char *path, int oflag, ...);
int close_real (int fd);

#if DYAD_SYNC_DIR
int sync_directory (const char* path);
#endif // DYAD_SYNC_DIR

/*****************************************************************************
 *                                                                           *
 *                   DYAD Sync Internal API                                  *
 *                                                                           *
 *****************************************************************************/

static inline bool is_dyad_producer ()
{
    char *e = NULL;
    if ((e = getenv (DYAD_KIND_PROD_ENV)))
        return (atoi (e) > 0);
    return false;
}

static inline bool is_dyad_consumer ()
{
    char *e = NULL;
    if ((e = getenv (DYAD_KIND_CONS_ENV)))
        return (atoi (e) > 0);
    return false;
}

int close_sync (const char *path)
{
    int rc = -1;
    const char *dyad_path = NULL;
    char upath [PATH_MAX] = {'\0'};

    if (!(dyad_path = getenv (DYAD_PATH_PROD_ENV))) {
        rc = 0;
        if ((dyad_path = getenv (DYAD_PATH_CONS_ENV))) {
            IPRINTF ("DYAD_SYNC CLOSE: no need to sync consumer close (\"%s\")\n", path);
        } else {
            IPRINTF ("DYAD_SYNC CLOSE not enabled for \"%s\".\n", path);
        }
        goto done;
    }

    if (cmp_canonical_path_prefix (dyad_path, path, upath, PATH_MAX)) {
        rc = dyad_close_sync (path, dyad_path, upath);
    } else {
        IPRINTF ("DYAD_SYNC CLOSE: %s is not a prefix of %s.\n", \
                 dyad_path, path);
        rc = 0;
    }

done:
    if (rc == 0 && (ctx && ctx->check))
        setenv (DYAD_CHECK_ENV, "ok", 1);

    return rc;
}


/*****************************************************************************
 *                                                                           *
 *         DYAD Sync Constructor, Destructor and Wrapper API                 *
 *                                                                           *
 *****************************************************************************/

void dyad_sync_init (void)
{
    char *e = NULL;

    bool debug = false;
    bool check = false;
    bool shared_storage = false;
    unsigned int key_depth = 0;
    unsigned int key_bins = 0;
    char *kvs_namespace = NULL;
    char *managed_path = NULL;

    DPRINTF ("DYAD_WRAPPER: Initializeing DYAD wrapper\n");

    if ((e = getenv ("DYAD_SYNC_DEBUG"))) {
        debug = true;
        enable_debug_dyad_utils ();
    } else {
        debug = false;
        disable_debug_dyad_utils ();
    }

    if ((e = getenv ("DYAD_SYNC_CHECK")))
        check = true;
    else
        check = false;

    if ((e = getenv ("DYAD_SHARED_STORAGE")))
        shared_storage = true;
    else
        shared_storage = false;

    if ((e = getenv ("DYAD_KEY_DEPTH")))
        key_depth = atoi (e);
    else
        key_depth = 3;

    if ((e = getenv ("DYAD_KEY_BINS")))
        key_bins = atoi (e);
    else
        key_bins = 1024;

    if ((e = getenv ("FLUX_KVS_NAMESPACE")))
        kvs_namespace = e;
    else
        kvs_namespace = NULL;

    if (is_dyad_consumer())
    {
        if ((e = getenv(DYAD_PATH_CONS_ENV)))
        {
            managed_path = e;
        }
        else
        {
            managed_path = NULL;
        }
    }
    else
    {
        if ((e = getenv(DYAD_PATH_PROD_ENV)))
        {
            managed_path = e;
        }
        else
        {
            managed_path = NULL;
        }
    }

    int rc = dyad_init(debug, check, shared_storage, key_depth, 
            key_bins, kvs_namespace, managed_path, &ctx);

    if (DYAD_IS_ERROR(rc))
    {
        FLUX_LOG_ERR("Could not initialize DYAD!\n");
        ctx = NULL;
        return;
    }

    FLUX_LOG_INFO ("DYAD Initialized\n");
    FLUX_LOG_INFO ("DYAD_SYNC_DEBUG=%s\n", (ctx->debug)? "true": "false");
    FLUX_LOG_INFO ("DYAD_SYNC_CHECK=%s\n", (ctx->check)? "true": "false");
    FLUX_LOG_INFO ("DYAD_KEY_DEPTH=%u\n", ctx->key_depth);
    FLUX_LOG_INFO ("DYAD_KEY_BINS=%u\n", ctx->key_bins);

  #if DYAD_SYNC_START
    ctx->sync_started = false;
    if ((e = getenv ("DYAD_SYNC_START")) && (atoi (e) > 0)) {
        FLUX_LOG_INFO ("Before barrier %u\n", ctx->rank);
        flux_future_t *fb;
        if (!(fb = flux_barrier (ctx->h, "sync_start", atoi (e))))
            FLUX_LOG_ERR ("flux_barrier failed for %d ranks\n", atoi (e));
        if (flux_future_get (fb, NULL) < 0)
            FLUX_LOG_ERR ("flux_future_get for barrir failed\n");
        FLUX_LOG_INFO ("After barrier %u\n", ctx->rank);
        flux_future_destroy (fb);

        ctx->sync_started = true;
        struct timespec t_now;
        clock_gettime (CLOCK_REALTIME, &t_now);
        char tbuf[100];
        strftime (tbuf, sizeof (tbuf), "%D %T", gmtime (&(t_now.tv_sec)));
        printf ("DYAD synchronized start at %s.%09ld\n",
                 tbuf, t_now.tv_nsec);
    }
  #endif // DYAD_SYNC_START
}

void dyad_sync_fini ()
{
    if (ctx == NULL)
    {
        return;
    }
#if DYAD_SYNC_START
    if (ctx->sync_started) {
        struct timespec t_now;
        clock_gettime (CLOCK_REALTIME, &t_now);
        char tbuf[100];
        strftime (tbuf, sizeof (tbuf), "%D %T", gmtime (&(t_now.tv_sec)));
        printf ("DYAD stops at %s.%09ld\n",
                 tbuf, t_now.tv_nsec);
    }
#endif // DYAD_SYNC_START
    dyad_finalize(ctx);
}

int open (const char *path, int oflag, ...)
{
    char *error = NULL;
    typedef int (*open_ptr_t) (const char *, int, mode_t, ...);
    open_ptr_t func_ptr = NULL;

    func_ptr = (open_ptr_t) dlsym (RTLD_NEXT, "open");
    if ((error = dlerror ())) {
        DPRINTF ("DYAD_SYNC: error in dlsym: %s\n", error);
        return -1;
    }

    if (is_path_dir (path)) {
        // TODO: make sure if the directory mode is consistent
        goto real_call;
    }

    if (!(ctx && ctx->h) || (ctx && !ctx->reenter)) {
        IPRINTF ("DYAD_SYNC: open sync not applicable for \"%s\".\n", path);
        goto real_call;
    }

    IPRINTF ("DYAD_SYNC: enters open sync (\"%s\").\n", path);
    int rc = dyad_consume(ctx, path);
    if (DYAD_IS_ERROR(rc)) {
        DPRINTF ("DYAD_SYNC: failed open sync (\"%s\").\n", path);
        goto real_call;
    }
    IPRINTF ("DYAD_SYNC: exists open sync (\"%s\").\n", path);

real_call:;
    int mode = 0;

    if (oflag & O_CREAT)
    {
        va_list arg;
        va_start (arg, oflag);
        mode = va_arg (arg, int);
        va_end (arg);
    }
    return (func_ptr (path, oflag, mode));
}

FILE *fopen (const char *path, const char *mode)
{
    char *error = NULL;
    typedef FILE *(*fopen_ptr_t) (const char *, const char *);
    fopen_ptr_t func_ptr = NULL;

    func_ptr = (fopen_ptr_t) dlsym (RTLD_NEXT, "fopen");
    if ((error = dlerror ())) {
        DPRINTF ("DYAD_SYNC: error in dlsym: %s\n", error);
        return NULL;
    }

    if (is_path_dir (path)) {
        // TODO: make sure if the directory mode is consistent
        goto real_call;
    }

    if (!(ctx && ctx->h) || (ctx && !ctx->reenter) || !path) {
        IPRINTF ("DYAD_SYNC: fopen sync not applicable for \"%s\".\n",
                 ((path)? path : ""));
        goto real_call;
    }

    IPRINTF ("DYAD_SYNC: enters fopen sync (\"%s\").\n", path);
    int rc = dyad_consume(ctx, path);
    if (DYAD_IS_ERROR(rc)) {
        DPRINTF ("DYAD_SYNC: failed fopen sync (\"%s\").\n", path);
        goto real_call;
    }
    IPRINTF ("DYAD_SYNC: exits fopen sync (\"%s\").\n", path);

real_call:
    return (func_ptr (path, mode));
}

int close (int fd)
{
    bool to_sync = false;
    char *error = NULL;
    typedef int (*close_ptr_t) (int);
    close_ptr_t func_ptr = NULL;
    char path [PATH_MAX+1] = {'\0'};
    int rc = 0;

    func_ptr = (close_ptr_t) dlsym (RTLD_NEXT, "close");
    if ((error = dlerror ())) {
        DPRINTF ("DYAD_SYNC: error in dlsym: %s\n", error);
        return -1; // return the failure code
    }

    if ((fd < 0) || (ctx == NULL) || (ctx->h == NULL) || !ctx->reenter) {
      #if defined(IPRINTF_DEFINED)
        if (ctx == NULL) {
            IPRINTF ("DYAD_SYNC: close sync not applicable. (no context)\n");
        } else if (ctx->h == NULL) {
            IPRINTF ("DYAD_SYNC: close sync not applicable. (no flux)\n");
        } else if (!ctx->reenter) {
            IPRINTF ("DYAD_SYNC: close sync not applicable. (no reenter)\n");
        } else if (fd >= 0) {
            IPRINTF ("DYAD_SYNC: close sync not applicable. (invalid file descriptor)\n");
        }
      #endif // defined(IPRINTF_DEFINED)
        goto real_call;
    }

    if (is_fd_dir (fd)) {
        // TODO: make sure if the directory mode is consistent
        goto real_call;
    }

    if (get_path (fd, PATH_MAX-1, path) < 0) {
        IPRINTF ("DYAD_SYNC: unable to obtain file path from a descriptor.\n");
        goto real_call;
    }

    to_sync = true;

real_call:; // semicolon here to avoid the error
    // "a label can only be part of a statement and a declaration is not a statement"
    fsync (fd);

  #if DYAD_SYNC_DIR
    if (to_sync) {
        sync_directory (path);
    }
  #endif // DYAD_SYNC_DIR

    rc = func_ptr (fd);

    if (to_sync) {
        if (rc != 0) {
            DPRINTF ("Failed close (\"%s\").: %s\n", path, strerror (errno));
        }
        IPRINTF ("DYAD_SYNC: enters close sync (\"%s\").\n", path);
        int dyad_rc = dyad_produce(ctx, path);
        if (DYAD_IS_ERROR(dyad_rc)) {
            DPRINTF ("DYAD_SYNC: failed close sync (\"%s\").\n", path);
        }
        IPRINTF ("DYAD_SYNC: exits close sync (\"%s\").\n", path);
    }

    return rc;
}

int fclose (FILE *fp)
{
    bool to_sync = false;
    char *error = NULL;
    typedef int (*fclose_ptr_t) (FILE *);
    fclose_ptr_t func_ptr = NULL;
    char path [PATH_MAX+1] = {'\0'};
    int rc = 0;

    func_ptr = (fclose_ptr_t) dlsym (RTLD_NEXT, "fclose");
    if ((error = dlerror ())) {
        DPRINTF ("DYAD_SYNC: error in dlsym: %s\n", error);
        return EOF; // return the failure code
    }

    if ((fp == NULL) || (ctx == NULL) || (ctx->h == NULL) || !ctx->reenter) {
      #if defined(IPRINTF_DEFINED)
        if (ctx == NULL) {
            IPRINTF ("DYAD_SYNC: fclose sync not applicable. (no context)\n");
        } else if (ctx->h == NULL) {
            IPRINTF ("DYAD_SYNC: fclose sync not applicable. (no flux)\n");
        } else if (!ctx->reenter) {
            IPRINTF ("DYAD_SYNC: fclose sync not applicable. (no reenter)\n");
        } else if (fp == NULL) {
            IPRINTF ("DYAD_SYNC: fclose sync not applicable. (invalid file pointer)\n");
        }
      #endif // defined(IPRINTF_DEFINED)
        goto real_call;
    }

    if (is_fd_dir (fileno (fp))) {
        // TODO: make sure if the directory mode is consistent
        goto real_call;
    }

    if (get_path (fileno (fp), PATH_MAX-1, path) < 0) {
        IPRINTF ("DYAD_SYNC: unable to obtain file path from a descriptor.\n");
        goto real_call;
    }

    to_sync = true;

real_call:;
    fflush (fp);
    fsync (fileno (fp));

  #if DYAD_SYNC_DIR
    if (to_sync) {
        sync_directory (path);
    }
  #endif // DYAD_SYNC_DIR

    rc = func_ptr (fp);

    if (to_sync) {
        if (rc != 0) {
            DPRINTF ("Failed fclose (\"%s\").\n", path);
        }
        IPRINTF ("DYAD_SYNC: enters fclose sync (\"%s\").\n", path);
        int dyad_rc = dyad_produce(ctx, path);
        if (DYAD_IS_ERROR(dyad_rc)) {
            DPRINTF ("DYAD_SYNC: failed fclose sync (\"%s\").\n", path);
        }
        IPRINTF ("DYAD_SYNC: exits fclose sync (\"%s\").\n", path);
    }

    return rc;
}

int open_real (const char *path, int oflag, ...)
{
    typedef int (*open_real_ptr_t) (const char *, int, mode_t, ...);
    open_real_ptr_t func_ptr = (open_real_ptr_t) dlsym (RTLD_NEXT, "open");
    char *error = NULL;

    if ((error = dlerror ())) {
        DPRINTF ("DYAD: open() error in dlsym: %s\n", error);
        return -1;
    }

    int mode = 0;

    if (oflag & O_CREAT)
    {
        va_list arg;
        va_start (arg, oflag);
        mode = va_arg (arg, int);
        va_end (arg);
    }

    return (func_ptr (path, oflag, mode));
}

int close_real (int fd)
{
    typedef int (*close_real_ptr_t) (int);
    close_real_ptr_t func_ptr = (close_real_ptr_t) dlsym (RTLD_NEXT, "close");
    char *error = NULL;

    if ((error = dlerror ())) {
        DPRINTF ("DYAD: close() error in dlsym: %s\n", error);
        return -1; // return the failure code
    }

    return func_ptr (fd);
}

#if DYAD_SYNC_DIR
int sync_directory (const char* path)
{ // Flush new directory entry https://lwn.net/Articles/457671/
    char path_copy [PATH_MAX+1] = {'\0'};
    int odir_fd = -1;
    char *odir = NULL;
    bool reenter = false;
    int rc = 0;

    strncpy (path_copy, path, PATH_MAX);
    odir = dirname (path_copy);

    reenter = ctx->reenter; // backup ctx->reenter
    if (ctx != NULL) ctx->reenter = false;

    if ((odir_fd = open_real (odir, O_RDONLY)) < 0) {
        IPRINTF ("Cannot open the directory \"%s\"\n", odir);
        rc = -1;
    } else {
        if (fsync (odir_fd) < 0) {
            IPRINTF ("Cannot flush the directory \"%s\"\n", odir);
            rc = -1;
        }
        if (close_real (odir_fd) < 0) {
            IPRINTF ("Cannot close the directory \"%s\"\n", odir);
            rc = -1;
        }
    }
    if (ctx != NULL) ctx->reenter = reenter;
    return rc;
}
#endif // DYAD_SYNC_DIR

/*
 * vi: ts=4 sw=4 expandtab
 */
