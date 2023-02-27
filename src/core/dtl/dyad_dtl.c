#include "dyad_dtl.h"

#include "dyad_dtl_defs.h"
#include "ucx_dtl.h"
#include "flux_dtl.h"

// Actual definition of dyad_dtl_t
struct dyad_dtl {
    dyad_dtl_mode_t mode;
    void *real_dtl_handle;
};

dyad_rc_t dyad_dtl_init(dyad_dtl_mode_t mode, flux_t *h,
        const char *kvs_namespace, bool debug,
        dyad_dtl_t **dtl_handle)
{
    *dtl_handle = malloc(sizeof(struct dyad_dtl));
    if (*dtl_handle == NULL)
    {
        FLUX_LOG_ERR (h, "Could not allocate a dyad_dtl_t object\n");
        return DYAD_RC_SYSFAIL;
    }
    (*dtl_handle)->mode = mode;
    if (mode == DYAD_DTL_UCX) {
        return dyad_dtl_ucx_init (h, kvs_namespace, debug,
                (dyad_dtl_ucx_t**)&(*dtl_handle)->real_dtl_handle);
    }
    if (mode == DYAD_DTL_FLUX_RPC)
    {
        return dyad_dtl_flux_init (h, kvs_namespace, debug,
                (dyad_dtl_flux_t**)&(*dtl_handle)->real_dtl_handle);
    }
    FLUX_LOG_ERR (h, "Invalid DYAD DTL Mode: %d\n", (int) mode);
    return DYAD_RC_BADDTLMODE;
}

dyad_rc_t dyad_dtl_establish_connection(dyad_dtl_t *dtl_handle,
        uint32_t producer_rank) {
    if (dtl_handle->mode == DYAD_DTL_UCX) {
        return dyad_dtl_ucx_establish_connection(
            (dyad_dtl_ucx_t*) dtl_handle->real_dtl_handle,
            producer_rank
        );
    }
    if (dtl_handle->mode == DYAD_DTL_FLUX_RPC) {
        return dyad_dtl_flux_establish_connection(
            (dyad_dtl_flux_t*) dtl_handle->real_dtl_handle,
            producer_rank
        );
    }
    FLUX_LOG_ERR (h, "Invalid DYAD DTL Mode: %d\n", (int) dtl_handle->mode);
    return DYAD_RC_BADDTLMODE;
}

dyad_rc_t dyad_dtl_rpc_pack(dyad_dtl_t *dtl_handle, const char *upath,
        json_t **packed_obj)
{
    if (dtl_handle->mode == DYAD_DTL_UCX) {
        return dyad_dtl_ucx_rpc_pack(
            (dyad_dtl_ucx_t*) dtl_handle->real_dtl_handle,
            upath,
            packed_obj
        );
    }
    if (dtl_handle->mode == DYAD_DTL_FLUX_RPC) {
        return dyad_dtl_flux_rpc_pack(
            (dyad_dtl_flux_t*) dtl_handle->real_dtl_handle,
            upath,
            packed_obj
        );
    }
    FLUX_LOG_ERR (h, "Invalid DYAD DTL Mode: %d\n", (int) dtl_handle->mode);
    return DYAD_RC_BADDTLMODE;
}

dyad_rc_t dyad_dtl_recv(dyad_dtl_t *dtl_handle, flux_future_t *f,
        void **buf, size_t *buflen)
{
    if (dtl_handle->mode == DYAD_DTL_UCX) {
        return dyad_dtl_ucx_recv(
            (dyad_dtl_ucx_t*) dtl_handle->real_dtl_handle,
            f,
            buf,
            buflen
        );
    }
    if (dtl_handle->mode == DYAD_DTL_FLUX_RPC) {
        return dyad_dtl_flux_recv(
            (dyad_dtl_flux_t*) dtl_handle->real_dtl_handle,
            f,
            buf,
            buflen
        );
    }
    FLUX_LOG_ERR (h, "Invalid DYAD DTL Mode: %d\n", (int) dtl_handle->mode);
    return DYAD_RC_BADDTLMODE;
}

dyad_rc_t dyad_dtl_close_connection(dyad_dtl_t *dtl_handle)
{
    if (dtl_handle->mode == DYAD_DTL_UCX) {
        return dyad_dtl_ucx_close_connection(
            (dyad_dtl_ucx_t*) dtl_handle->real_dtl_handle
        );
    }
    if (dtl_handle->mode == DYAD_DTL_FLUX_RPC) {
        return dyad_dtl_flux_close_connection(
            (dyad_dtl_flux_t*) dtl_handle->real_dtl_handle
        );
    }
    FLUX_LOG_ERR (h, "Invalid DYAD DTL Mode: %d\n", (int) dtl_handle->mode);
    return DYAD_RC_BADDTLMODE;
}

dyad_rc_t dyad_dtl_finalize(dyad_dtl_t *dtl_handle)
{
    if (dtl_handle->mode == DYAD_DTL_UCX) {
        return dyad_dtl_ucx_finalize(
            (dyad_dtl_ucx_t*) dtl_handle->real_dtl_handle
        );
    }
    if (dtl_handle->mode == DYAD_DTL_FLUX_RPC) {
        return dyad_dtl_flux_finalize(
            (dyad_dtl_flux_t*) dtl_handle->real_dtl_handle
        );
    }
    FLUX_LOG_ERR (h, "Invalid DYAD DTL Mode: %d\n", (int) dtl_handle->mode);
    return DYAD_RC_BADDTLMODE;
}
