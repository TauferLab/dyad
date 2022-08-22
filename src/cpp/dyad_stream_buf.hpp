#ifndef DYAD_STREAM_BUF_HPP
#define DYAD_STREAM_BUF_HPP

#include "dyad_stream_core.hpp"
#include <fstream>

namespace dyad {

    template <class CharT, class Traits = std::char_traits<CharT>>
    class dyad_filebuf : public std::basic_filebuf<CharT, Traits>
    {
        public:

            using dyad_fbuf = typename dyad::dyad_filebuf<CharT, Traits>;
            using basic_fbuf = typename std::basic_filebuf<CharT, Traits>;

            dyad_filebuf(dyad_stream_core *ctx);
            dyad_filebuf();
#if __cplusplus >= 201103L
            dyad_filebuf(const dyad_fbuf& rhs) = delete;
            dyad_filebuf(dyad_fbuf&& rhs);

            dyad_fbuf& operator=(dyad_fbuf&& rhs);
            dyad_fbuf& operator=(dyad_fbuf& rhs) = delete;

            void swap(dyad_fbuf& rhs);
#endif
            virtual ~dyad_filebuf();
            
            dyad_fbuf* open(const char *s, std::ios_base::openmode mode);
#if __cplusplus >= 201103L
            dyad_fbuf* open(const std::string& str,
                    std::ios_base::openmode mode);
#if (__cplusplus >= 201703L) && __has_include(<filesystem>)
            dyad_fbuf* open(const std::filesystem::path& p,
                    std::ios_base::openmode mode);
            typename std::enable_if<!std::is_same<std::filesystem::path::value_type, char>::value, dyad_fbuf*>
            open(const std::filesystem::path::value_type* s,
                    std::ios_base::openmode mode);
#endif /* 201703L */
#endif /* 201103L */
            dyad_fbuf* close();

            void init();

            void init(const dyad_params& p);

            dyad_stream_core* move_dyad_core();

        protected:

            dyad_stream_core *m_ctx;
            bool m_self_constructed_ctx;
            std::string m_filename;
    };

    template <class CharT, class Traits>
    dyad_filebuf<CharT, Traits>::dyad_filebuf(dyad_stream_core *ctx)
    : std::basic_filebuf<CharT, Traits>()
    , m_ctx(ctx)
    , m_self_constructed_ctx(false)
    {}

    template <class CharT, class Traits>
    dyad_filebuf<CharT, Traits>::dyad_filebuf()
    : std::basic_filebuf<CharT, Traits>()
    , m_self_constructed_ctx(true)
    {
        m_ctx = new dyad_stream_core();
    }

#if __cplusplus >= 201103L
    template <class CharT, class Traits>
    dyad_filebuf<CharT, Traits>::dyad_filebuf(dyad_filebuf<CharT, Traits>&& rhs)
    : m_ctx(std::move(rhs.m_ctx))
    , m_self_constructed_ctx(std::move(rhs.m_self_constructed_ctx))
    , std::basic_filebuf<CharT, Traits>(std::move(rhs))
    {}

    template <class CharT, class Traits>
    dyad_filebuf<CharT, Traits>& dyad_filebuf<CharT, Traits>::operator=(dyad_fbuf&& rhs)
    {
        close();
        m_ctx = std::move(rhs.m_ctx);
        m_self_constructed_ctx = std::move(rhs.m_self_constructed_ctx);
        std::basic_filebuf<CharT, Traits>::operator=(std::move(rhs));
        return (*this);
    }

    template <class CharT, class Traits>
    void dyad_filebuf<CharT, Traits>::swap(dyad_fbuf& rhs)
    {
        dyad_stream_core *tmp_ctx = m_ctx;
        m_ctx = rhs.m_ctx;
        rhs.m_ctx = tmp_ctx;
        bool tmp_constructed_ctx = m_self_constructed_ctx;
        m_self_constructed_ctx = rhs.m_self_constructed_ctx;
        rhs.m_self_constructed_ctx = tmp_constructed_ctx;
        std::string tmp_filename = m_filename;
        m_filename = rhs.m_filename;
        rhs.m_filename = tmp_filename;
        std::basic_filebuf<CharT, Traits>::swap(rhs);
    }
#endif /* 201103L */

    template <class CharT, class Traits>
    dyad_filebuf<CharT, Traits>::~dyad_filebuf()
    {
        try 
        {
            close();
        } 
        catch (...) 
        {
        }
        if (m_ctx != NULL)
        {
            delete m_ctx;
            m_ctx = NULL;
        }
    }
    
    template <class CharT, class Traits>
    dyad_filebuf<CharT, Traits>* dyad_filebuf<CharT, Traits>::open(
            const char *s, std::ios_base::openmode mode)
    {
        if (!m_ctx->m_initialized)
        {
            m_ctx->init();
        }
        m_ctx->set_mode(mode);
        m_filename = std::string(s);
        if (m_ctx->is_dyad_consumer())
        {
            m_ctx->open_sync(s);
        }
        if (std::basic_filebuf<CharT, Traits>::open(s, mode) == NULL)
        {
            return NULL;
        }
        return this;
    }

#if __cplusplus >= 201103L
    template <class CharT, class Traits>
    dyad_filebuf<CharT, Traits>* dyad_filebuf<CharT, Traits>::open(
            const std::string& str, std::ios_base::openmode mode)
    {
        return open(str.c_str(), mode);
    }
#if (__cplusplus >= 201703L) && __has_include(<filesystem>)
    template <class CharT, class Traits>
    dyad_filebuf<CharT, Traits>* dyad_filebuf<CharT, Traits>::open(
            const std::filesystem::path& p, std::ios_base::openmode mode)
    {
        return open(p.c_str(), mode);
    }

    template <class CharT, class Traits>
    typename std::enable_if<!std::is_same<std::filesystem::path::value_type, char>::value, dyad_filebuf<CharT, Traits>*>
    dyad_filebuf<CharT, Traits>::open(
            const std::filesystem::path::value_type* s,
            std::ios_base::openmode mode)
    {
        if (!m_ctx->m_initialized)
        {
            m_ctx->init();
        }
        m_ctx->set_mode(mode);
        std::filesystem::path p = s;
        m_filename = p.string();
        if (m_ctx->is_dyad_consumer())
        {
            m_ctx->open_sync(m_filename.c_str());
        }
        if (std::basic_filebuf<CharT, Traits>::open(s, mode) == NULL)
        {
            return NULL;
        }
        return this;
    }
#endif /* 201703L */
#endif /* 201103L */

    template <class CharT, class Traits>
    dyad_filebuf<CharT, Traits>* dyad_filebuf<CharT, Traits>::close()
    {
        if (m_ctx->is_dyad_producer())
        {
            if (m_filename == "")
            {
                // TODO log error
            }
            else 
            {
                m_ctx->close_sync(m_filename.c_str());
            }
        }
        if (std::basic_filebuf<CharT, Traits>::close() == NULL)
        {
            return NULL;
        }
        return this;
    }

    template <class CharT, class Traits>
    void dyad_filebuf<CharT, Traits>::init()
    {
        m_ctx->init();
    }

    template <class CharT, class Traits>
    void dyad_filebuf<CharT, Traits>::init(const dyad_params& p)
    {
        m_ctx->init(p);
    }

    template <class CharT, class Traits>
    dyad_stream_core* dyad_filebuf<CharT, Traits>::move_dyad_core()
    {
        dyad_stream_core* tmp = m_ctx;
        m_ctx = NULL;
        return tmp;
    }

}

#endif /* DYAD_STREAM_BUF_HPP */
