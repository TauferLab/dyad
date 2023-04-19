#ifndef __UCX_DTL_H__
#define __UCX_DTL_H__

#include "dyad_flux_log.h"
#include "dyad_rc.h"

#include <na.h>
#include <jansson.h>

#ifdef __cplusplus
#include <cstdlib>
#else
#include <stdlib.h>
#endif

struct dyad_dtl_mercury {
    // Data for client/consumer
    flux_t *h;
    const char *kvs_namespace;
    na_class_t *mercury_class;
    na_context_t *mercury_ctx;
    na_addr_t *mercury_addr;
    // Data for current connection/transfer
    na_tag_t mercury_tag;
    na_mem_handle_t *remote_mem_handle;
   na_addr_t *remote_addr;
    size_t data_size;
};

typedef struct dyad_dtl_mercury dyad_dtl_mercury_t;

dyad_rc_t dyad_dtl_mercury_init(flux_t *h, const char *kvs_namespace,
        bool debug, dyad_dtl_mercury_t **dtl_handle);

dyad_rc_t dyad_dtl_mercury_rpc_pack(dyad_dtl_mercury_t *dtl_handle,
        const char *upath, uint32_t producer_rank, json_t **packed_obj);

dyad_rc_t dyad_dtl_mercury_recv_rpc_response(dyad_dtl_mercury_t *dtl_handle,
        flux_future_t *f);

dyad_rc_t dyad_dtl_mercury_establish_connection(dyad_dtl_mercury_t *dtl_handle);

dyad_rc_t dyad_dtl_mercury_recv(dyad_dtl_mercury_t *dtl_handle,
        void **buf, size_t *buflen);

dyad_rc_t dyad_dtl_mercury_close_connection(
        dyad_dtl_mercury_t *dtl_handle);

dyad_rc_t dyad_dtl_mercury_finalize(dyad_dtl_mercury_t *dtl_handle);

#endif /* __UCX_DTL_H__ */
