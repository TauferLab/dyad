/************************************************************\
 * Copyright 2021 Lawrence Livermore National Security, LLC
 * (c.f. AUTHORS, NOTICE.LLNS, COPYING)
 *
 * This file is part of the Flux resource manager framework.
 * For details, see https://github.com/flux-framework.
 *
 * SPDX-License-Identifier: LGPL-3.0
\************************************************************/

#ifndef DYAD_MOD_CTX_H
#define DYAD_MOD_CTX_H

#include <stdbool.h>
#include <flux/core.h>
#include <ucp/api/ucp.h>

struct dyad_mod_ctx {
    flux_t *h;
    bool debug;
    flux_msg_handler_t **handlers;
    const char *dyad_path;
    ucp_context_h ucx_ctx;
    ucp_worker_h ucx_worker;
} dyad_mod_ctx_default =
    {NULL, false, NULL, NULL, NULL, NULL};

typedef struct dyad_mod_ctx dyad_mod_ctx_t;

#endif // DYAD_MOD_CTX_H
