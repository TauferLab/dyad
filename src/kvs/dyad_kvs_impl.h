#ifndef DYAD_KVS_DYAD_KVS_IMPL_H
#define DYAD_KVS_DYAD_KVS_IMPL_H

#include "dyad_flux_log.h"
#include "dyad_kvs.h"
#include "dyad_rc.h"

#ifdef __cplusplus
#include <cstding>

extern "C" {
#else
#include <stdint.h>
#endif

struct dyad_metadata;

struct dyad_kvs_flux;

union dyad_kvs_private {
    struct dyad_kvs_flux* flux_kvs_handle;
};
typedef union dyad_kvs_private dyad_kvs_private_t;

struct dyad_kvs {
    dyad_kvs_private_t private;
    dyad_kvs_mode_t mode;
    dyad_rc_t (*kvs_read) (struct dyad_kvs* restrict self,
                           const char* restrict key,
                           bool should_wait,
                           struct dyad_metadata** restrict mdata);
    dyad_rc_t (*kvs_write) (struct dyad_kvs* restrict self,
                            const char* restrict key,
                            const struct dyad_metadata* restrict mdata);
};

dyad_rc_t dyad_kvs_init (dyad_kvs_t** kvs_handle, dyad_kvs_mode_t mode, flux_t* h, bool debug);

dyad_rc_t dyad_kvs_finalize (dyad_kvs_t** kvs_handle);

#ifdef __cplusplus
}
#endif

#endif /* DYAD_KVS_DYAD_KVS_IMPL_H */