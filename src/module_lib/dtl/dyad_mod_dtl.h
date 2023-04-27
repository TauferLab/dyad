#ifndef __DYAD_MOD_DTL_H__
#define __DYAD_MOD_DTL_H__

#include <dyad_flux_log.h>
#include <dyad_dtl_defs.h>

#if defined(__cplusplus)
#include <cstdlib>
#else
#include <stdbool.h>
#include <stdlib.h>
#endif /* defined(__cplusplus) */

int dyad_mod_dtl_init(dyad_dtl_mode_t mode, flux_t *h,
        bool debug, dyad_dtl_t **dtl_handle);

int dyad_mod_dtl_rpc_unpack(dyad_dtl_t *dtl_handle,
        const flux_msg_t *packed_obj, char **upath);

int dyad_mod_dtl_rpc_respond(dyad_dtl_t *dtl_handle,
        const flux_msg_t *orig_msg);

int dyad_mod_dtl_establish_connection(dyad_dtl_t *dtl_handle);

int dyad_mod_dtl_send(dyad_dtl_t *dtl_handle, void *buf, size_t buflen);

int dyad_mod_dtl_close_connection(dyad_dtl_t *dtl_handle);

int dyad_mod_dtl_finalize(dyad_dtl_t **dtl_handle);

#endif /* __DYAD_MOD_DTL_H__ */
