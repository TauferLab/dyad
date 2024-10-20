#ifndef DYAD_COMMON_DYAD_ENVS_H
#define DYAD_COMMON_DYAD_ENVS_H

#if defined(DYAD_HAS_CONFIG)
#include <dyad/dyad_config.hpp>
#else
#error "no config"
#endif

#define DYAD_PATH_PRODUCER_ENV "DYAD_PATH_PRODUCER"
#define DYAD_PATH_CONSUMER_ENV "DYAD_PATH_CONSUMER"
#define DYAD_KVS_NAMESPACE_ENV "DYAD_KVS_NAMESPACE"
#define DYAD_DTL_MODE_ENV "DYAD_DTL_MODE"
#define DYAD_SHARED_STORAGE_ENV "DYAD_SHARED_STORAGE"
#define DYAD_KEY_DEPTH_ENV "DYAD_KEY_DEPTH"
#define DYAD_KEY_BINS_ENV "DYAD_KEY_BINS"
#define DYAD_CHECK_ENV "DYAD_SYNC_HEALTH"
#define DYAD_SYNC_CHECK_ENV "DYAD_SYNC_CHECK"
#define DYAD_SYNC_DEBUG_ENV "DYAD_SYNC_DEBUG"
#define DYAD_SERVICE_MUX_ENV "DYAD_SERVICE_MUX"
#define DYAD_REINIT_ENV "DYAD_REINIT"

#endif  // DYAD_COMMON_DYAD_ENVS_H
