#include "flux_dtl.h"
#include "dyad_err.h"

dyad_core_err_t dyad_dtl_flux_init(flux_t *h, const char *kvs_namespace,
        bool debug, dyad_dtl_flux_t **dtl_handle)
{
    *dtl_handle = malloc(sizeof(struct dyad_dtl_flux));
    if (*dtl_handle == NULL)
    {
        FLUX_LOG_ERR (h, "Cannot allocate the DTL handle for Flux\n");
        return DYAD_SYSFAIL;
    }
    (*dtl_handle)->h = h;
    (*dtl_handle)->kvs_namespace = kvs_namespace;
    return DYAD_OK;
}

dyad_core_err_t dyad_dtl_flux_establish_connection(dyad_dtl_flux_t *dtl_handle,
        uint32_t producer_rank)
{
    return DYAD_OK;
}

dyad_core_err_t dyad_dtl_flux_rpc_pack(dyad_dtl_flux_t *dtl_handle, const char *upath,
        json_t **packed_obj)
{
    *packed_obj = json_pack(
        "{s:s}",
        "upath",
        upath
    );
    if (*packed_obj == NULL)
    {
        FLUX_LOG_ERR (dtl_handle->h, "Could not pack upath for Flux DTL\n");
        return DYAD_BADPACK;
    }
    return DYAD_OK;
}

dyad_core_err_t dyad_dtl_flux_recv(dyad_dtl_flux_t *dtl_handle, flux_future_t *f,
        void **buf, size_t *buflen)
{
    int rc = flux_rpc_get_raw(f, (const void**) buf, buflen);
    if (f != NULL)
    {
        flux_future_destroy(f);
        f = NULL;
    }
    if (rc < 0)
    {
        FLUX_LOG_ERR (dtl_handle->h, "Could not get file data from Flux RPC\n");
        return DYAD_BADRPC;
    }
    return DYAD_OK;
}

dyad_core_err_t dyad_dtl_flux_close_connection(dyad_dtl_flux_t *dtl_handle)
{
    return DYAD_OK;
}

dyad_core_err_t dyad_dtl_flux_finalize(dyad_dtl_flux_t *dtl_handle)
{
    if (dtl_handle != NULL)
    {
        dtl_handle->h = NULL;
        dtl_handle->kvs_namespace = NULL;
        free(dtl_handle);
        dtl_handle = NULL;
    }
    return DYAD_OK;
}
