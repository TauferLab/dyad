#ifndef __DYAD_MOD_mercury_dtl_H__
#define __DYAD_MOD_mercury_dtl_H__

#include "dyad_flux_log.h"
#include <na.h>

#if defined(__cplusplus)
#include <cstdlib>
#else
#include <stdbool.h>
#include <stdlib.h>
#endif

struct dyad_mod_mercury_dtl {
    flux_t *h;
    na_class_t *mercury_class;
    na_context_t *mercury_ctx;
    na_addr_t *mercury_addr;
    // Per tranfer data
    const flux_msg_t *rpc_msg;
    na_tag_t mercury_tag;
    na_addr_t *consumer_addr;
};

typedef struct dyad_mod_mercury_dtl dyad_mod_mercury_dtl_t;

int dyad_mod_mercury_dtl_init(flux_t *h, bool debug, dyad_mod_mercury_dtl_t **dtl_handle);

int dyad_mod_mercury_dtl_rpc_unpack(dyad_mod_mercury_dtl_t *dtl_handle,
        const flux_msg_t *packed_obj, char **upath);

int dyad_mod_mercury_dtl_rpc_respond (dyad_mod_mercury_dtl_t *dtl_handle,
        const flux_msg_t *orig_msg);

int dyad_mod_mercury_dtl_establish_connection(dyad_mod_mercury_dtl_t *dtl_handle);

int dyad_mod_mercury_dtl_send(dyad_mod_mercury_dtl_t *dtl_handle, void *buf, size_t buflen);

int dyad_mod_mercury_dtl_close_connection(dyad_mod_mercury_dtl_t *dtl_handle);

int dyad_mod_mercury_dtl_finalize(dyad_mod_mercury_dtl_t *dtl_handle);

#endif /* __DYAD_MOD_mercury_dtl_H__ */
