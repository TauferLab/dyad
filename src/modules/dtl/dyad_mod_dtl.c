#include "dyad_mod_dtl.h"

#include "ucx_mod_dtl.h"
#include "flux_mod_dtl.h"

// SWITCH_OVER_DTL_MODES is copied from the Core DTL sublibrary
//
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

struct dyad_mod_dtl {
    dyad_mod_dtl_mode_t mode;
    void *real_handle;
};

int dyad_mod_dtl_init(dyad_mod_dtl_mode_t mode, flux_t *h,
        bool debug, dyad_mod_dtl_t **dtl_handle)
{
    *dtl_handle = malloc(sizeof(struct dyad_mod_dtl));
    if (*dtl_handle == NULL)
    {
        // TODO log error
        return -1;
    }
    (*dtl_handle)->mode = mode;
    SWITCH_OVER_DTL_MODES(mode,
        // UCX DTL
        {
            return dyad_mod_ucx_dtl_init(
                h,
                debug,
                (dyad_mod_ucx_dtl_t**)&(*dtl_handle)->real_handle
            );
        },
        // FLUX RPC DTL
        {
            return dyad_mod_flux_dtl_init(
                h,
                debug,
                (dyad_mod_flux_dtl_t**)&(*dtl_handle)->real_handle
            );
        },
        // Default
        {
            // TODO log error
            return -1;
        }
    )
}

int dyad_mod_dtl_rpc_unpack(dyad_mod_dtl_t *dtl_handle,
        flux_msg_t *packed_obj, char **upath)
{
    SWITCH_OVER_DTL_MODES(dtl_handle->mode,
        // UCX DTL
        {
            return dyad_mod_ucx_dtl_rpc_unpack(
                (dyad_mod_ucx_dtl_t*)dtl_handle->real_handle,
                packed_obj,
                upath
            );
        },
        // FLUX RPC DTL
        {
            return dyad_mod_flux_dtl_rpc_unpack(
                (dyad_mod_flux_dtl_t*)dtl_handle->real_handle,
                packed_obj,
                upath
            );
        },
        // Default
        {
            // TODO log error
            return -1;
        }
    )
}

int dyad_mod_dtl_establish_connection(dyad_mod_dtl_t *dtl_handle)
{
    SWITCH_OVER_DTL_MODES(dtl_handle->mode,
        // UCX DTL
        {
            return dyad_mod_ucx_dtl_establish_connection(
                (dyad_mod_ucx_dtl_t*)dtl_handle->real_handle
            );
        },
        // FLUX RPC DTL
        {
            return dyad_mod_flux_dtl_establish_connection(
                (dyad_mod_flux_dtl_t*)dtl_handle->real_handle
            );
        },
        // Default
        {
            // TODO log error
            return -1;
        }
    )
}

int dyad_mod_dtl_send(dyad_mod_dtl_t *dtl_handle, void *buf, size_t buflen)
{
    SWITCH_OVER_DTL_MODES(dtl_handle->mode,
        // UCX DTL
        {
            return dyad_mod_ucx_dtl_send(
                (dyad_mod_ucx_dtl_t*)dtl_handle->real_handle,
                buf,
                buflen
            );
        },
        // FLUX RPC DTL
        {
            return dyad_mod_flux_dtl_send(
                (dyad_mod_flux_dtl_t*)dtl_handle->real_handle,
                buf,
                buflen
            );
        },
        // Default
        {
            // TODO log error
            return -1;
        }
    )
}

int dyad_mod_dtl_close_connection(dyad_mod_dtl_t *dtl_handle)
{
    SWITCH_OVER_DTL_MODES(dtl_handle->mode,
        // UCX DTL
        {
            return dyad_mod_ucx_dtl_close_connection(
                (dyad_mod_ucx_dtl_t*)dtl_handle->real_handle
            );
        },
        // FLUX RPC DTL
        {
            return dyad_mod_flux_dtl_close_connection(
                (dyad_mod_flux_dtl_t*)dtl_handle->real_handle
            );
        },
        // Default
        {
            // TODO log error
            return -1;
        }
    )
}

int dyad_mod_dtl_finalize(dyad_mod_dtl_t *dtl_handle)
{
    SWITCH_OVER_DTL_MODES(dtl_handle->mode,
        // UCX DTL
        {
            return dyad_mod_ucx_dtl_finalize(
                (dyad_mod_ucx_dtl_t*)dtl_handle->real_handle
            );
        },
        // FLUX RPC DTL
        {
            return dyad_mod_flux_dtl_finalize(
                (dyad_mod_flux_dtl_t*)dtl_handle->real_handle
            );
        },
        // Default
        {
            // TODO log error
            return -1;
        }
    )
}
