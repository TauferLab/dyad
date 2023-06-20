#ifndef DYAD_FSTREAM_CORE_HPP
#define DYAD_FSTREAM_CORE_HPP

#if __cplusplus >= 201103L
#include <utility>
#endif

#include "dyad_core.h"
#include "dyad_params.hpp"

namespace dyad
{

class dyad_fstream_core
{
    public:

        dyad_fstream_core ();
#if __cplusplus >= 201103L
        dyad_fstream_core (const dyad_fstream_core &rhs) = delete;
        dyad_fstream_core (dyad_fstream_core &&rhs);
#endif

        ~dyad_fstream_core ();

#if __cplusplus >= 201103L
        dyad_fstream_core& operator= (dyad_fstream_core &&rhs);
#endif

        void init ();
        void init (const dyad_params& p);
        void log_info (const std::string &msg_head) const;

        void finalize ();

        bool is_dyad_producer ();
        bool is_dyad_consumer ();

        bool open_sync (const char *path);
        bool close_sync (const char *path);

        bool check_initialized () const;

    private:

        dyad_ctx_t *m_ctx;
        bool m_is_prod;
        bool m_is_cons;

};

dyad_fstream_core::dyad_fstream_core ()
    : m_ctx (NULL)
    , m_is_prod (false)
    , m_is_cons (false)
{
}

#if __cplusplus >= 201103L

dyad_fstream_core::dyad_fstream_core (dyad_fstream_core &&rhs)
    : m_ctx (std::move (rhs.m_ctx))
    , m_is_prod (std::move (rhs.m_is_prod))
    , m_is_cons (std::move (rhs.m_is_cons))
{
    rhs.m_ctx = nullptr;
    rhs.m_is_prod = false;
    rhs.m_is_cons = false;
}

#endif

dyad_fstream_core::~dyad_fstream_core ()
{
    finalize();
}

#if __cplusplus >= 201103L

dyad_fstream_core& dyad_fstream_core::operator= (dyad_fstream_core &&rhs)
{
    if (this == &rhs)
        return *this;
    m_ctx = std::move (rhs.m_ctx);
    rhs.m_ctx = nullptr;
    m_is_prod = std::move (rhs.m_is_prod);
    rhs.m_is_prod = false;
    m_is_cons = std::move (rhs.m_is_cons);
    rhs.m_is_cons = false;
    return *this;
}

#endif

void dyad_fstream_core::finalize ()
{
    if (m_ctx != NULL) {
        dyad_finalize (&m_ctx);
        m_ctx = NULL;
    }
}

void dyad_fstream_core::init ()
{
    if (m_ctx != NULL && m_ctx->initialized)
        return;
    dyad_rc_t rc = dyad_init_env (&m_ctx);
    if (rc == DYAD_RC_OK) {
        if (m_ctx->prod_managed_path != NULL) {
            m_is_prod = (strlen (m_ctx->prod_managed_path) != 0);
        } else {
            m_is_prod = false;
        }
        if (m_ctx->cons_managed_path != NULL) {
            m_is_cons = (strlen (m_ctx->cons_managed_path) != 0);
        } else {
            m_is_cons = false;
        }
    }
    log_info ("Fstream core is initialized by env variables");
}

void dyad_fstream_core::init (const dyad_params &p)
{
    dyad_rc_t rc = dyad_init (p.m_debug, false, p.m_shared_storage, p.m_key_depth,
            p.m_key_bins, p.m_kvs_namespace.c_str (),
            p.m_prod_managed_path.c_str (),
            p.m_cons_managed_path.c_str (),
            static_cast<dyad_dtl_mode_t>(p.m_dtl_mode),
            &m_ctx);
    if (!DYAD_IS_ERROR (rc)) {
        if (m_ctx->prod_managed_path != NULL) {
            m_is_prod = (strlen (m_ctx->prod_managed_path) != 0);
        } else {
            m_is_prod = false;
        }
        if (m_ctx->cons_managed_path != NULL) {
            m_is_cons = (strlen (m_ctx->cons_managed_path) != 0);
        } else {
            m_is_cons = false;
        }
    }
    log_info ("Fstream core is initialized by parameters");
}

void dyad_fstream_core::log_info (const std::string &msg_head) const
{
    if (m_ctx == NULL)
        return;
    DYAD_LOG_INFO (m_ctx, "=== %s ===\n", msg_head.c_str ());
    DYAD_LOG_INFO (m_ctx, "%s=%s\n", DYAD_PATH_CONSUMER_ENV,
                   m_ctx->cons_managed_path);
    DYAD_LOG_INFO (m_ctx, "%s=%s\n", DYAD_PATH_PRODUCER_ENV,
                   m_ctx->prod_managed_path);
    DYAD_LOG_INFO (m_ctx, "%s=%s\n", DYAD_SYNC_DEBUG_ENV,
                   (m_ctx->debug) ? "true" : "false");
    DYAD_LOG_INFO (m_ctx, "%s=%s\n", DYAD_SHARED_STORAGE_ENV,
                   (m_ctx->shared_storage) ? "true" : "false");
    DYAD_LOG_INFO (m_ctx, "%s=%u\n", DYAD_KEY_DEPTH_ENV, m_ctx->key_depth);
    DYAD_LOG_INFO (m_ctx, "%s=%u\n", DYAD_KEY_BINS_ENV, m_ctx->key_bins);
    DYAD_LOG_INFO (m_ctx, "%s=%s\n", DYAD_KVS_NAMESPACE_ENV,
                   m_ctx->kvs_namespace);
}

bool dyad_fstream_core::is_dyad_producer ()
{
    return m_is_prod;
}

bool dyad_fstream_core::is_dyad_consumer ()
{
    return m_is_cons;
}

bool dyad_fstream_core::open_sync (const char *path)
{
    IPRINTF (m_ctx, "DYAD_FSTREAM: entering open_sync (\"%s\")\n", path);
    if (m_ctx == NULL || !m_ctx->initialized)
        return true;
    if (!is_dyad_consumer())
        return true;
    dyad_rc_t rc = dyad_consume (m_ctx, path);
    if (DYAD_IS_ERROR (rc)) {
        DPRINTF (m_ctx, "DYAD_FSTREAM: open_sync failed (\"%s\")\n", path);
        return false;
    }
    IPRINTF (m_ctx, "DYAD_FSTREAM: open_sync succeeded (\"%s\")\n", path);
    return true;
}

bool dyad_fstream_core::close_sync (const char *path)
{
    IPRINTF (m_ctx, "DYAD_FSTREAM: entering close_sync (\"%s\")\n", path);
    if (m_ctx == NULL || !m_ctx->initialized)
        return true;
    if (!is_dyad_producer())
        return true;
    dyad_rc_t rc = dyad_produce (m_ctx, path);
    if (DYAD_IS_ERROR (rc)) {
        DPRINTF (m_ctx, "DYAD_FSTREAM: close_sync failed (\"%s\")\n", path);
        return false;
    }
    IPRINTF (m_ctx, "DYAD_FSTREAM: close_sync succeeded (\"%s\")\n", path);
    return true;
}

bool dyad_fstream_core::check_initialized () const
{
    if (m_ctx == NULL)
        return false;
    return m_ctx->initialized;
}

}

#endif /* DYAD_FSTREAM_CORE_HPP */
