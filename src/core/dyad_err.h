#ifndef __DYAD_CORE_DYAD_ERR_H__
#define __DYAD_CORE_DYAD_ERR_H__

enum dyad_core_err_codes {
    DYAD_OK = 0, // Operation worked correctly
    DYAD_SYSFAIL = -1, // Some sys call or C standard
                       // library call failed
    DYAD_NOCTX = -2, // No DYAD Context found
    DYAD_FLUXFAIL = -3, // Some Flux function failed
    DYAD_BADCOMMIT = -4, // Flux KVS commit didn't work
    DYAD_BADLOOKUP = -5, // Flux KVS lookup didn't work
    DYAD_BADFETCH = -6, // Flux KVS commit didn't work
    DYAD_BADRESPONSE = -7, // Cannot create/populate a DYAD response
    DYAD_BADRPC = -8, // Flux RPC pack or get didn't work
    DYAD_BADFIO = -9, // File I/O failed
    DYAD_BADMANAGEDPATH = -10, // Cons or Prod Manged Path is bad
    DYAD_BADPACK = -11, // JSON payload packing failed
    DYAD_UCXINIT_FAIL = -12, // UCX initialization failed
    DYAD_UCXWAIT_FAIL = -13, // UCX wait (either custom or
                             // 'ucp_worker_wait') failed
    DYAD_UCXCOMM_FAIL = -14, // UCX communication routine failed
};

typedef enum dyad_core_err_codes dyad_core_err_t;

#define DYAD_IS_ERROR(code) (code != DYAD_OK)

#endif
