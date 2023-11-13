#include "dyad_kvs_impl.h"

#include "dyad_flux_kvs.h"

dyad_rc_t dyad_kvs_init (dyad_kvs_t** kvs_handle, dyad_kvs_mode_t mode, flux_t h, bool debug)
{
    dyad_rc_t rc = DYAD_RC_OK;
    *kvs_handle = malloc (sizeof (struct dyad_kvs));
    if (*kvs_handle == NULL) {
        rc = DYAD_RC_SYSFAIL;
        goto kvs_init_done;
    }
    (*kvs_handle)->mode = mode;
    if (mode == DYAD_KVS_FLUX) {
        rc = dyad_kvs_flux_init (*kvs_handle, mode, h, debug);
        if (DYAD_IS_ERROR (rc)) {
            goto kvs_init_done;
        }
    } else {
        rc = DYAD_RC_BADKVSMODE;
        goto kvs_init_done;
    }
    rc = DYAD_RC_OK;

kvs_init_done:
    return rc;
}

dyad_rc_t dyad_kvs_finalize (dyad_kvs_t** kvs_handle)
{
    dyad_rc_t rc = DYAD_RC_OK;
    if (kvs_handle == NULL || *kvs_handle == NULL) {
        rc = DYAD_RC_OK;
        goto kvs_finalize_done;
    }
    if ((*kvs_handle)->mode == DYAD_KVS_FLUX) {
        rc = dyad_kvs_flux_finalize (kvs_handle);
        if (DYAD_IS_ERROR (rc)) {
            goto kvs_finalize_done;
        }
    } else {
        rc = DYAD_RC_BADKVSMODE;
        goto kvs_finalize_done;
    }
    rc = DYAD_RC_OK;

kvs_finalize_done:
    free (*kvs_handle);
    *kvs_handle = NULL;
    return rc;
}