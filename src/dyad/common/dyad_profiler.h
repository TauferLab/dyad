#ifndef DYAD_COMMON_DYAD_PROFILER_H
#define DYAD_COMMON_DYAD_PROFILER_H
#if defined(DYAD_HAS_CONFIG)
#include <dyad/dyad_config.hpp>
#else
#error "no config"
#endif
#ifdef DYAD_PROFILER_DLIO_PROFILER // DLIO_PROFILER
#include <dlio_profiler/dlio_profiler.h>
#endif

#ifdef __cplusplus
#ifdef DYAD_PROFILER_NONE
#define DYAD_CPP_FUNCTION() ;
#define DYAD_CPP_FUNCTION_UPDATE(key, value) ;
#define DYAD_CPP_REGION_START(name) ;
#define DYAD_CPP_REGION_END(name) ;
#define DYAD_CPP_REGION_UPDATE(name) ;
#elif defined(DYAD_PROFILER_PERFFLOW_ASPECT) // PERFFLOW_ASPECT
#define DYAD_CPP_FUNCTION() ;
#define DYAD_CPP_FUNCTION_UPDATE(key, value) ;
#define DYAD_CPP_REGION_START(name) ;
#define DYAD_CPP_REGION_END(name) ;
#define DYAD_CPP_REGION_UPDATE(name) ;
#elif defined(DYAD_PROFILER_CALIPER) // CALIPER
#define DYAD_CPP_FUNCTION() ;
#define DYAD_CPP_FUNCTION_UPDATE(key, value) ;
#define DYAD_CPP_REGION_START(name) ;
#define DYAD_CPP_REGION_END(name) ;
#define DYAD_CPP_REGION_UPDATE(name) ;
#elif defined(DYAD_PROFILER_DLIO_PROFILER) // DLIO_PROFILER
#define DYAD_CPP_FUNCTION() DLIO_PROFILER_CPP_FUNCTION();
#define DYAD_CPP_FUNCTION_UPDATE(key, value) DLIO_PROFILER_CPP_FUNCTION_UPDATE(key, value);
#define DYAD_CPP_REGION_START(name) DLIO_PROFILER_CPP_REGION_START(name);
#define DYAD_CPP_REGION_END(name) DLIO_PROFILER_CPP_REGION_END(name);
#define DYAD_CPP_REGION_UPDATE(name) DLIO_PROFILER_CPP_REGION_DYN_UPDATE(name, key, value);
#endif


extern "C" {
#endif
#ifdef DYAD_PROFILER_NONE
#define DYAD_C_FUNCTION_START() ;
#define DYAD_C_FUNCTION_END() ;
#define DYAD_C_FUNCTION_UPDATE_INT(key, value) ;
#define DYAD_C_FUNCTION_UPDATE_STR(key, value) ;
#define DYAD_C_REGION_START(name) ;
#define DYAD_C_REGION_END(name) ;
#define DYAD_C_REGION_UPDATE_INT(name) ;
#define DYAD_C_REGION_UPDATE_STR(name) ;
#elif defined(DYAD_PROFILER_PERFFLOW_ASPECT) // PERFFLOW_ASPECT
#define DYAD_C_FUNCTION_START() ;
#define DYAD_C_FUNCTION_END() ;
#define DYAD_C_FUNCTION_UPDATE_INT(key, value) ;
#define DYAD_C_FUNCTION_UPDATE_STR(key, value) ;
#define DYAD_C_REGION_START(name) ;
#define DYAD_C_REGION_END(name) ;
#define DYAD_C_REGION_UPDATE_INT(name) ;
#define DYAD_C_REGION_UPDATE_STR(name) ;
#elif defined(DYAD_PROFILER_CALIPER) // CALIPER
#define DYAD_C_FUNCTION_START() ;
#define DYAD_C_FUNCTION_END() ;
#define DYAD_C_FUNCTION_UPDATE_INT(key, value) ;
#define DYAD_C_FUNCTION_UPDATE_STR(key, value) ;
#define DYAD_C_REGION_START(name) ;
#define DYAD_C_REGION_END(name) ;
#define DYAD_C_REGION_UPDATE_INT(name) ;
#define DYAD_C_REGION_UPDATE_STR(name) ;
#elif defined(DYAD_PROFILER_DLIO_PROFILER) // DLIO_PROFILER
#define DYAD_C_FUNCTION_START() DLIO_PROFILER_C_FUNCTION_START();
#define DYAD_C_FUNCTION_END() DLIO_PROFILER_C_FUNCTION_END();
#define DYAD_C_FUNCTION_UPDATE_INT(key, value) DLIO_PROFILER_C_FUNCTION_UPDATE_INT(key, value);
#define DYAD_C_FUNCTION_UPDATE_STR(key, value) DLIO_PROFILER_C_FUNCTION_UPDATE_STR(key, value);
#define DYAD_C_REGION_START(name) DLIO_PROFILER_C_REGION_START(name);
#define DYAD_C_REGION_END(name) DLIO_PROFILER_C_REGION_END(name);
#define DYAD_C_REGION_UPDATE_INT(name) DLIO_PROFILER_C_REGION_UPDATE_INT(name, key, value);
#define DYAD_C_REGION_UPDATE_STR(name) DLIO_PROFILER_C_REGION_UPDATE_STR(name, key, value);
#endif

#ifdef __cplusplus
}
#endif

#endif  // DYAD_COMMON_DYAD_PROFILER_H
