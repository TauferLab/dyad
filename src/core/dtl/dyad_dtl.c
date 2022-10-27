#include "dyad_dtl.h"

#include "ucx_dtl.h"
#include "flux_dtl.h"

int dyad_dtl_init(flux_t *h, const char *kvs_namespace,
        bool debug, dyad_dtl_t **dtl_handle)
{
#if DYAD_DTL_UCX_ENABLE
    return dyad_dtl_ucx_init(
        h,
        kvs_namespace,
        debug,
        (dyad_dtl_ucx_t**) dtl_handle
    );
#else
    return dyad_dtl_flux_init(
        h,
        kvs_namespace,
        debug,
        (dyad_dtl_flux_t**) dtl_handle
    );
#endif
}

int dyad_dtl_establish_connection(dyad_dtl_t *dtl_handle,
        uint32_t producer_rank)
{
#if DYAD_DTL_UCX_ENABLE
    return dyad_dtl_ucx_establish_connection(
        (dyad_dtl_ucx_t*) dtl_handle,
        producer_rank
    );
#else
    return dyad_dtl_flux_establish_connection(
        (dyad_dtl_flux_t*) dtl_handle,
        producer_rank
    );
#endif
}

int dyad_dtl_rpc_pack(dyad_dtl_t *dtl_handle, const char *upath,
        json_t **packed_obj)
{
#if DYAD_DTL_UCX_ENABLE
    return dyad_dtl_ucx_rpc_pack(
        (dyad_dtl_ucx_t*) dtl_handle,
        upath,
        packed_obj
    );
#else
    return dyad_dtl_flux_rpc_pack(
        (dyad_dtl_flux_t*) dtl_handle,
        upath,
        packed_obj
    );
#endif
}

int dyad_dtl_recv(dyad_dtl_t *dtl_handle, flux_future_t *f,
        void **buf, size_t *buflen)
{
#if DYAD_DTL_UCX_ENABLE
    return dyad_dtl_ucx_recv(
        (dyad_dtl_ucx_t*) dtl_handle,
        f,
        buf,
        buflen
    );
#else
    return dyad_dtl_flux_recv(
        (dyad_dtl_flux_t*) dtl_handle,
        f,
        buf,
        buflen
    );
#endif
}

int dyad_dtl_close_connection(dyad_dtl_t *dtl_handle)
{
#if DYAD_DTL_UCX_ENABLE
    return dyad_dtl_ucx_close_connection(
        (dyad_dtl_ucx_t*) dtl_handle
    );
#else
    return dyad_dtl_flux_close_connection(
        (dyad_dtl_flux_t*) dtl_handle
    );
#endif
}

int dyad_dtl_finalize(dyad_dtl_t *dtl_handle)
{
#if DYAD_DTL_UCX_ENABLE
    return dyad_dtl_ucx_finalize(
        (dyad_dtl_ucx_t*) dtl_handle
    );
#else
    return dyad_dtl_flux_finalize(
        (dyad_dtl_flux_t*) dtl_handle
    );
#endif
}
