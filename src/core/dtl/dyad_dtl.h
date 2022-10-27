#ifndef __DYAD_DTL_H__
#define __DYAD_DTL_H__

#include "dyad_dtl_defs.h"
#include "dyad_err.h"

#include <jansson.h>

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

dyad_dtl_err_t dyad_dtl_init(flux_t *h, const char *kvs_namespace,
        bool debug, dyad_dtl_t **dtl_handle);

dyad_dtl_err_t dyad_dtl_establish_connection(dyad_dtl_t *dtl_handle,
        uint32_t producer_rank);

dyad_dtl_err_t dyad_dtl_rpc_pack(dyad_dtl_t *dtl_handle,
        const char *upath, json_t **packed_obj);

dyad_dtl_err_t dyad_dtl_recv(dyad_dtl_t *dtl_handle, flux_future_t *f,
        void **buf, size_t *buflen);

dyad_dtl_err_t dyad_dtl_close_connection(dyad_dtl_t *dtl_handle);

dyad_dtl_err_t dyad_dtl_finalize(dyad_dtl_t *dtl_handle);

#endif /* __DYAD_DTL_H__ */
