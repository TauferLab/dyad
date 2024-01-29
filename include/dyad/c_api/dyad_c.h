#ifndef DYAD_C_API_DYAD_C_H
#define DYAD_C_API_DYAD_C_H

#if defined(DYAD_HAS_CONFIG)
#include <dyad/dyad_config.hpp>
#else
#error "no config"
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct dyad_ctx;

__attribute__ ((__visibility__ ("default"))) int dyad_c_init (struct dyad_ctx** ctx);

__attribute__ ((__visibility__ ("default"))) int dyad_c_open (struct dyad_ctx* ctx,
                                                              const char* path,
                                                              int oflag,
                                                              ...);

__attribute__ ((__visibility__ ("default"))) int dyad_c_close (struct dyad_ctx* ctx, int fd);

__attribute__ ((__visibility__ ("default"))) int dyad_c_finalize (struct dyad_ctx** ctx);

#ifdef __cplusplus
}
#endif

#endif /* DYAD_C_API_DYAD_C_H */