#ifndef __FLUX_DTL_H__
#define __FLUX_DTL_H__

#include "dyad_flux_log.h"

#include <jansson.h>

#ifdef __cplusplus
#include <cstdlib>
#else
#include <stdlib.h>
#endif

struct dyad_dtl_flux {
    flux_t *h;
    const char *kvs_namespace;
};

typedef struct dyad_dtl_flux dyad_dtl_flux_t;

int dyad_dtl_flux_init(flux_t *h, const char *kvs_namespace,
        bool debug, dyad_dtl_flux_t **dtl_handle);

int dyad_dtl_flux_establish_connection(dyad_dtl_flux_t *dtl_handle,
       uint32_t producer_rank);

int dyad_dtl_flux_rpc_pack(dyad_dtl_flux_t *dtl_handle, const char *upath,
        json_t **packed_obj);

int dyad_dtl_flux_recv(dyad_dtl_flux_t *dtl_handle, flux_future_t *f,
        void **buf, size_t *buflen);

int dyad_dtl_flux_close_connection(dyad_dtl_flux_t *dtl_handle);

int dyad_dtl_flux_finalize(dyad_dtl_flux_t *dtl_handle);

#endif /* __FLUX_DTL_H__ */
