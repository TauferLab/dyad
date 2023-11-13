#ifndef DYAD_KVS_DYAD_FLUX_KVS_H
#define DYAD_KVS_DYAD_FLUX_KVS_H

#include "dyad_kvs_impl.h"

struct dyad_kvs_flux {
    flux_t* h;
    bool debug;
    char* kvs_namespace;
};
typedef struct dyad_kvs_flux dyad_kvs_flux_t;

dyad_rc_t dyad_kvs_flux_init (dyad_kvs_t** kvs_handle,
                              dyad_kvs_mode_t mode,
                              const char* restrict kvs_namespace,
                              flux_t* h,
                              bool debug);

dyad_rc_t dyad_kvs_flux_read (dyad_kvs_t* restrict self,
                              const char* restrict key,
                              bool should_wait,
                              dyad_metadata_t** restrict mdata);

dyad_rc_t dyad_kvs_flux_write (dyad_kvs_t* restrict self,
                               const char* restrict key,
                               const dyad_metadata_t* restrict mdata);

dyad_rc_t dyad_kvs_flux_finalize (dyad_kvs_t** self);

#endif /* DYAD_KVS_DYAD_FLUX_KVS_H */