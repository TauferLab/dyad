#ifndef DYAD_KVS_DYAD_KVS_H
#define DYAD_KVS_DYAD_KVS_H

#ifdef __cplusplus
extern "C" {
#endif

enum dyad_kvs_mode { DYAD_KVS_FLUX = 0, DYAD_KVS_END = 2 };

typedef enum dyad_kvs_mode dyad_kvs_mode_t;

static const dyad_kvs_mode_t DYAD_KVS_DEFAULT = DYAD_KVS_FLUX;

struct dyad_kvs;

#ifdef __cplusplus
}
#endif

#endif /* DYAD_KVS_DYAD_KVS_H */