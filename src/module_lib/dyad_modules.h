#ifndef __DYAD_MODULE_LIB_MODULES_H__
#define __DYAD_MODULE_LIB_MODULES_H__

#include <flux/core.h>

typedef struct dyad_mod_ctx dyad_mod_ctx_t;

typedef int (*dyad_module_get_data_size_t)(const char *key,
        size_t *data_size, void *user_data);

typedef int (*dyad_module_get_t)(const char *key, void *buf,
        size_t buflen, void *user_data);

typedef int (*dyad_module_free_user_data_t)(void **user_data);

dyad_mod_ctx_t *dyad_module_get_ctx (flux_t *h);

int dyad_module_open (flux_t *h, dyad_dtl_mode_t dtl_mode, bool debug);

void dyad_module_set_data_size_get_cb (dyad_mod_ctx_t *ctx,
        dyad_module_get_data_size_t size_getter_cb);

void dyad_module_set_data_get_cb (dyad_mod_ctx_t *ctx,
        dyad_module_get_t data_getter_cb);

void dyad_module_set_user_data (dyad_mod_ctx_t *ctx, void *user_data,
        dyad_module_free_user_data_t free_cb);

extern const struct flux_msg_handler_spec dyad_module_callbacks[];

int dyad_module_run (dyad_mod_ctx_t *ctx);

#endif /* __DYAD_MODULE_LIB_MODULES_H__ */
