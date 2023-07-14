#ifndef DYAD_DTL_H
#define DYAD_DTL_H

#include "dyad_rc.h"

#include <flux.h>
#include <jansson.h>

#ifdef __cplusplus
#include <cstdint>

extern "C" {
#else
#include <stdint.h>
#endif
    
enum dyad_dtl_mode {
    DYAD_DTL_UCX = 0,
    DYAD_DTL_FLUX_RPC = 1,
};
typedef enum dyad_dtl_mode dyad_dtl_mode_t;

struct dyad_dtl_impl;

struct dyad_dtl {
    struct dyad_dtl_impl* impl;
    dyad_rc_t (*rpc_pack)(const char*, uint32_t, json_t**);
    dyad_rc_t (*rpc_unpack)(const flux_msg_t*, char**);
    dyad_rc_t (*rpc_respond)(const flux_mst_t*);
    dyad_rc_t (*rpc_recv_response)(flux_future_t*);
    dyad_rc_t (*establish_connection)();
    dyad_rc_t (*send)(void*, size_t);
    dyad_rc_t (*recv)(void**, size_t*);
    dyad_rc_t (*close_connection)();
};
typedef struct dyad_dtl dyad_dtl_t;

dyad_rc_t dyad_dtl_init(dyad_dtl_t **dtl_handle,
        dyad_dtl_mode mode, flux_t *h, bool debug, ...);

dyad_rc_t dyad_dtl_finalize(dyad_dtl_t **dtl_handle);

#ifdef __cplusplus
}
#endif

#endif /* DYAD_DTL_H */