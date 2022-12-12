#include "dyad_dtl.h"

#include "dyad_dtl_defs.h"
#include "ucx_dtl.h"
#include "flux_dtl.h"

// To add extra cases, do the following:
//   1. Add an argument directly before 'default_case' in this macro
//   2. Add a case statement in the macro using the following template:
//      ```
//      case <ENUM_VAL>:\
//          <NEW_MACRO_ARGUMENT>\
//          break;\
//      ```
//   3. In each call of SWITCH_OVER_DTL_MODES, add a new positional argument for
//      the new case using the following template:
//      ```
//      {
//          <YOUR CODE HERE!>
//      },
//      ```
#define SWITCH_OVER_DTL_MODES(\
            mode,\
            ucx_case,\
            flux_rpc_case,\
            default_case\
        )\
    switch (mode)\
    {\
        case DYAD_DTL_UCX:\
            ucx_case\
            break;\
        case DYAD_DTL_FLUX_RPC:\
            flux_rpc_case\
            break;\
        default:\
            default_case\
    }

// Actual definition of dyad_dtl_t
struct dyad_dtl {
    dyad_dtl_mode_t mode;
    void *real_dtl_handle;
};

dyad_core_err_t dyad_dtl_init(dyad_dtl_mode_t mode, flux_t *h,
        const char *kvs_namespace, bool debug,
        dyad_dtl_t **dtl_handle)
{
    *dtl_handle = malloc(sizeof(struct dyad_dtl));
    if (*dtl_handle == NULL)
    {
        FLUX_LOG_ERR (h, "Could not allocate a dyad_dtl_t object\n");
        return -1;
    }
    (*dtl_handle)->mode = mode;
    SWITCH_OVER_DTL_MODES(mode,
        // UCX DTL
        {
            return dyad_dtl_ucx_init(
                h,
                kvs_namespace,
                debug,
                (dyad_dtl_ucx_t**)&(*dtl_handle)->real_dtl_handle
            );
        },
        // Flux RPC DTL
        {
            return dyad_dtl_flux_init(
                h,
                kvs_namespace,
                debug,
                (dyad_dtl_flux_t**)&(*dtl_handle)->real_dtl_handle
            );
        },
        // Default case
        {
            FLUX_LOG_ERR (h, "Invalid DYAD DTL Mode: %d\n", (int) mode);
            return -1;
        }
    )
}

dyad_core_err_t dyad_dtl_establish_connection(dyad_dtl_t *dtl_handle,
        uint32_t producer_rank)
{
    SWITCH_OVER_DTL_MODES(dtl_handle->mode,
        // UCX DTL
        {
            return dyad_dtl_ucx_establish_connection(
                (dyad_dtl_ucx_t*) dtl_handle->real_dtl_handle,
                producer_rank
            );
        },
        // Flux RPC DTL
        {
            return dyad_dtl_flux_establish_connection(
                (dyad_dtl_flux_t*) dtl_handle->real_dtl_handle,
                producer_rank
            );
        },
        // Default case
        {
            FLUX_LOG_ERR (h, "Invalid DYAD DTL Mode: %d\n", (int) dtl_handle->mode);
            return -1;
        }
    )
}

dyad_core_err_t dyad_dtl_rpc_pack(dyad_dtl_t *dtl_handle, const char *upath,
        json_t **packed_obj)
{
    SWITCH_OVER_DTL_MODES(dtl_handle->mode,
        // UCX DTL
        {
            return dyad_dtl_ucx_rpc_pack(
                (dyad_dtl_ucx_t*) dtl_handle->real_dtl_handle,
                upath,
                packed_obj
            );
        },
        // Flux RPC DTL
        {
            return dyad_dtl_flux_rpc_pack(
                (dyad_dtl_flux_t*) dtl_handle->real_dtl_handle,
                upath,
                packed_obj
            );
        },
        // Default case
        {
            FLUX_LOG_ERR (h, "Invalid DYAD DTL Mode: %d\n", (int) dtl_handle->mode);
            return -1;
        }
    )
}

dyad_core_err_t dyad_dtl_recv(dyad_dtl_t *dtl_handle, flux_future_t *f,
        void **buf, size_t *buflen)
{
    SWITCH_OVER_DTL_MODES(dtl_handle->mode,
        // UCX DTL
        {
            return dyad_dtl_ucx_recv(
                (dyad_dtl_ucx_t*) dtl_handle->real_dtl_handle,
                f,
                buf,
                buflen
            );
        },
        // Flux RPC DTL
        {
            return dyad_dtl_flux_recv(
                (dyad_dtl_flux_t*) dtl_handle->real_dtl_handle,
                f,
                buf,
                buflen
            );
        },
        // Default case
        {
            FLUX_LOG_ERR (h, "Invalid DYAD DTL Mode: %d\n", (int) dtl_handle->mode);
            return -1;
        }
    )
}

dyad_core_err_t dyad_dtl_close_connection(dyad_dtl_t *dtl_handle)
{
    SWITCH_OVER_DTL_MODES(dtl_handle->mode,
        // UCX DTL
        {
            return dyad_dtl_ucx_close_connection(
                (dyad_dtl_ucx_t*) dtl_handle->real_dtl_handle
            );
        },
        // Flux RPC DTL
        {
            return dyad_dtl_flux_close_connection(
                (dyad_dtl_flux_t*) dtl_handle->real_dtl_handle
            );
        },
        // Default case
        {
            FLUX_LOG_ERR (h, "Invalid DYAD DTL Mode: %d\n", (int) dtl_handle->mode);
            return -1;
        }
    )
}

dyad_core_err_t dyad_dtl_finalize(dyad_dtl_t *dtl_handle)
{
    SWITCH_OVER_DTL_MODES(dtl_handle->mode,
        // UCX DTL
        {
            return dyad_dtl_ucx_finalize(
                (dyad_dtl_ucx_t*) dtl_handle->real_dtl_handle
            );
        },
        // Flux RPC DTL
        {
            return dyad_dtl_flux_finalize(
                (dyad_dtl_flux_t*) dtl_handle->real_dtl_handle
            );
        },
        // Default case
        {
            FLUX_LOG_ERR (h, "Invalid DYAD DTL Mode: %d\n", (int) dtl_handle->mode);
            return -1;
        }
    )
}
