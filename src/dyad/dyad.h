#ifndef DYAD_CORE_DYAD_CORE_H
#define DYAD_CORE_DYAD_CORE_H

// Includes <flux/core.h>
#include "dyad_envs.h"
#include "dyad_rc.h"
#include "dyad_flux_log.h"
#include "dyad_dtl_defs.h"

#ifdef __cplusplus
#include <cstdio>
#include <cstdlib>
#else
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#endif

/*****************************************************************************
 *                                                                           *
 *                          DYAD Macro Definitions                           *
 *                                                                           *
 *****************************************************************************/

// Now defined in src/utils/utils.h
//#define DYAD_PATH_DELIM    "/"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct dyad_ctx
 */
struct dyad_ctx {
    flux_t* h;                // the Flux handle for DYAD
    dyad_dtl_t *dtl_handle;   // Opaque handle to DTL info
    bool debug;               // if true, perform debug logging
    bool check;               // if true, perform some check logging
    bool initialized;         // if true, DYAD is initialized
    unsigned int key_depth;   // Depth of bins for the Flux KVS
    unsigned int key_bins;    // Number of bins for the Flux KVS
    uint32_t rank;            // Flux rank for DYAD
    char* kvs_namespace;      // Flux KVS namespace for DYAD
};
extern const struct dyad_ctx dyad_ctx_default;
typedef struct dyad_ctx dyad_ctx_t;

struct dyad_kvs_response {
    char* fpath;
    uint32_t owner_rank;
};
typedef struct dyad_kvs_response dyad_kvs_response_t;

// Debug message
#ifndef DPRINTF
#define DPRINTF(curr_dyad_ctx, fmt, ...)           \
    do {                                           \
        if (curr_dyad_ctx && curr_dyad_ctx->debug) \
            fprintf (stderr, fmt, ##__VA_ARGS__);  \
    } while (0)
#endif  // DPRINTF

#define TIME_DIFF(Tstart, Tend)                                                \
    ((double)(1000000000L * ((Tend).tv_sec - (Tstart).tv_sec) + (Tend).tv_nsec \
              - (Tstart).tv_nsec)                                              \
     / 1000000000L)

// Detailed information message that can be omitted
#if DYAD_FULL_DEBUG
#define IPRINTF DPRINTF
#define IPRINTF_DEFINED
#else
#define IPRINTF(curr_dyad_ctx, fmt, ...)
#endif  // DYAD_FULL_DEBUG

dyad_rc_t dyad_init (bool debug,
                     bool check,
                     unsigned int key_depth,
                     unsigned int key_bins,
                     const char* kvs_namespace,
                     dyad_dtl_mode_t dtl_mode,
                     dyad_ctx_t** ctx);

dyad_rc_t dyad_init_env (dyad_ctx_t** ctx);

dyad_rc_t dyad_register (dyad_ctx_t *ctx, const char *key);

dyad_rc_t dyad_unregister (dyad_ctx_t *ctx, const char *key);

dyad_rc_t dyad_get_size (dyad_ctx_t *ctx, const char *key, size_t *data_size);

dyad_rc_t dyad_get (dyad_ctx_t *ctx, const char *key, void *buf, size_t buflen);

dyad_rc_t dyad_get_all (dyad_ctx_t *ctx, const char *key, void **buf, size_t *buflen);

dyad_rc_t dyad_finalize (dyad_ctx_t **ctx);

#ifdef __cplusplus
}
#endif

#endif /* DYAD_CORE_DYAD_CORE */
