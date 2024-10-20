#ifndef DYAD_COMMON_DYAD_DTL_H
#define DYAD_COMMON_DYAD_DTL_H

#if defined(DYAD_HAS_CONFIG)
#include <dyad/dyad_config.hpp>
#else
#error "no config"
#endif

#include <dyad/common/dyad_structures.h>
#ifdef __cplusplus
extern "C" {
#endif


enum dyad_dtl_mode { DYAD_DTL_UCX = 0, DYAD_DTL_FLUX_RPC = 1, DYAD_DTL_DEFAULT = 1, DYAD_DTL_END = 2 };
typedef enum dyad_dtl_mode dyad_dtl_mode_t;

static const char* dyad_dtl_mode_name[DYAD_DTL_END] __attribute__((unused)) = {"UCX", "FLUX_RPC"};

#define DYAD_DTL_RPC_NAME "dyad.fetch"

struct dyad_dtl;

#ifdef __cplusplus
}
#endif

#endif /* DYAD_COMMON_DYAD_DTL_H */
