#ifndef __DYAD_DTL_DEFS_H__
#define __DYAD_DTL_DEFS_H__

typedef struct dyad_dtl dyad_dtl_t;

enum dyad_dtl_mode {
    DYAD_DTL_FLUX_RPC,
    DYAD_DTL_UCX,
    // TODO Figure out how to use Flux RPC
    // as a fallback for if UCX fails
    // DYAD_DTL_UCX_W_FALLBACK,
};

typedef enum dyad_dtl_mode dyad_dtl_mode_t;

#endif /* __DYAD_DTL_DEFS_H__ */
