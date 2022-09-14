#ifndef DYAD_FSTREAM_HPP
#define DYAD_FSTREAM_HPP

#include "dyad_ifstream.hpp"
#include "dyad_stream_buf.hpp"
#include "dyad_stream_core.hpp"
#include <ios>
#include <iostream>
#include <istream>
#include <string>

namespace dyad {

    template <class CharT, class Traits = std::char_traits<CharT>>
    class dyad_basic_fstream : public std::basic_iostream<CharT, Traits> {
        public:

            dyad_basic_fstream();
            dyad_basic_fstream(dyad_stream_core *ctx);
            dyad_basic_fstream(const char *filename,
                    std::ios_base::openmode mode=std::ios_base::in | std::ios_base::out);
            dyad_basic_fstream(dyad_stream_core *ctx,
                    const char *filename,
                    std::ios_base::openmode mode=std::ios_base::in | std::ios_base::out);
#if __cplusplus >= 201103L
            dyad_basic_fstream(const std::string& filename,
                    std::ios_base::openmode mode=std::ios_base::in | std::ios_base::out);
            dyad_basic_fstream(dyad_stream_core *ctx,
                    const std::string& filename,
                    std::ios_base::openmode mode=std::ios_base::in | std::ios_base::out);
#if (__cplusplus >= 201703L) && __has_include(<filesystem>)
            template <typename OpenCharType = std::filesystem::path::value_type,
                      typename = std::enable_if<!std::is_same<OpenCharType, char>::value, void>>
            dyad_basic_fstream(const std::filesystem::path::value_type* filename,
                    std::ios_base::openmode mode=std::ios_base::in | std::ios_base::out);
            template <typename OpenCharType = std::filesystem::path::value_type,
                      typename = std::enable_if<!std::is_same<OpenCharType, char>::value, void>>
            dyad_basic_fstream(dyad_stream_core *ctx,
                    const std::filesystem::path::value_type* filename,
                    std::ios_base::openmode mode=std::ios_base::in | std::ios_base::out);
            dyad_basic_fstream(const std::filesystem::path& filename,
                    std::ios_base::openmode mode=std::ios_base::in | std::ios_base::out);
            dyad_basic_fstream(dyad_stream_core *ctx,
                    const std::filesystem::path& filename,
                    std::ios_base::openmode mode=std::ios_base::in | std::ios_base::out);
#endif /* 201703L */

            dyad_basic_fstream(const dyad_basic_fstream<CharT, Traits> &other) = delete;
            dyad_basic_fstream(dyad_basic_fstream<CharT, Traits> &&other);
            
            dyad_basic_fstream<CharT, Traits>& operator=(const dyad_basic_ifstream<CharT, Traits>&) = delete;
            dyad_basic_fstream<CharT, Traits>& operator=(dyad_basic_fstream<CharT, Traits>&& other);

            void swap(dyad_basic_fstream<CharT, Traits>& rhs);

            bool is_open() const;
#endif /* 201103L */
#if __cplusplus < 201103L
            bool is_open();
#endif

            dyad_filebuf<CharT, Traits>* rdbuf() const;

            void open(const char* filename,
                    std::ios_base::openmode mode=std::ios_base::in | std::ios_base::out);
#if __cplusplus >= 201103L
            void open(const std::string& filename,
                    std::ios_base::openmode mode=std::ios_base::in | std::ios_base::out);
#if (__cplusplus >= 201703L) && (__has_include(<filesystem>))
            template <typename OpenCharType = std::filesystem::path::value_type>
            typename std::enable_if<!std::is_same<OpenCharType, char>::value, void>::type 
            open(const std::filesystem::path::value_type* filename,
                    std::ios_base::openmode mode=std::ios_base::in | std::ios_base::out);
            void open(const std::filesystem::path& filename,
                    std::ios_base::openmode mode=std::ios_base::in | std::ios_base::out);
#endif /* 201703L and <filesystem> */
#endif /* 201103L */

            void close();

            void init();

            void init(const dyad_params& p);

            dyad_stream_core* move_stream_core();

        protected:

            dyad_filebuf<CharT, Traits> m_buf;

    };

#if __cplusplus >= 201103L
    template <class CharT, class Traits>
    void swap(dyad_basic_fstream<CharT, Traits>& x,
            dyad_basic_fstream<CharT, Traits>& y)
    {
        x.swap(y);
    }
#endif

    template <class CharT, class Traits>
    dyad_basic_fstream<CharT, Traits>::dyad_basic_fstream()
    : m_buf()
    , std::basic_iostream<CharT, Traits>(&m_buf)
    {}

    template <class CharT, class Traits>
    dyad_basic_fstream<CharT, Traits>::dyad_basic_fstream(
            dyad_stream_core *ctx)
    : m_buf(ctx)
    , std::basic_iostream<CharT, Traits>(&m_buf)
    {}

    template <class CharT, class Traits>
    dyad_basic_fstream<CharT, Traits>::dyad_basic_fstream(
            const char *filename,
            std::ios_base::openmode mode)
    : dyad_basic_fstream<CharT, Traits>()
    {
        dyad_filebuf<CharT, Traits>* ret_ptr = rdbuf()->open(filename, mode);
        if (ret_ptr == NULL)
        {
            std::basic_iostream<CharT, Traits>::setstate(std::ios_base::failbit);
        }
    }

    template <class CharT, class Traits>
    dyad_basic_fstream<CharT, Traits>::dyad_basic_fstream(
            dyad_stream_core *ctx,
            const char *filename,
            std::ios_base::openmode mode)
    : dyad_basic_fstream<CharT, Traits>(ctx)
    {
        dyad_filebuf<CharT, Traits>* ret_ptr = rdbuf()->open(filename, mode);
        if (ret_ptr == NULL)
        {
            std::basic_iostream<CharT, Traits>::setstate(std::ios_base::failbit);
        }
    }

#if __cplusplus >= 201103L

    template <class CharT, class Traits>
    dyad_basic_fstream<CharT, Traits>::dyad_basic_fstream(
            const std::string& filename,
            std::ios_base::openmode mode)
    : dyad_basic_fstream<CharT, Traits>(filename.c_str(), mode)
    {}

    template <class CharT, class Traits>
    dyad_basic_fstream<CharT, Traits>::dyad_basic_fstream(
            dyad_stream_core *ctx,
            const std::string& filename,
            std::ios_base::openmode mode)
    : dyad_basic_fstream<CharT, Traits>(ctx, filename.c_str(), mode)
    {}

#if (__cplusplus >= 201703L) && __has_include(<filesystem>)

    template <class CharT, class Traits>
    template <typename OpenCharType, typename>
    dyad_basic_fstream<CharT, Traits>::dyad_basic_fstream(
            const std::filesystem::path::value_type* filename,
            std::ios_base::openmode mode)
    : dyad_basic_fstream<CharT, Traits>()
    {
        dyad_filebuf<CharT, Traits>* ret_ptr = rdbuf()->open(filename, mode);
        if (ret_ptr == NULL)
        {
            std::basic_iostream<CharT, Traits>::setstate(std::ios_base::failbit);
        }
    }

    template <class CharT, class Traits>
    template <typename OpenCharType, typename>
    dyad_basic_fstream<CharT, Traits>::dyad_basic_fstream(
            dyad_stream_core *ctx,
            const std::filesystem::path::value_type* filename,
            std::ios_base::openmode mode)
    : dyad_basic_fstream<CharT, Traits>(ctx)
    {
        dyad_filebuf<CharT, Traits>* ret_ptr = rdbuf()->open(filename, mode);
        if (ret_ptr == NULL)
        {
            std::basic_iostream<CharT, Traits>::setstate(std::ios_base::failbit);
        }
    }

    template <class CharT, class Traits>
    dyad_basic_fstream<CharT, Traits>::dyad_basic_fstream(
            const std::filesystem::path& filename,
            std::ios_base::openmode mode)
    : dyad_basic_fstream<CharT, Traits>(filename.c_str(), mode)
    {}

    template <class CharT, class Traits>
    dyad_basic_fstream<CharT, Traits>::dyad_basic_fstream(
            dyad_stream_core *ctx,
            const std::filesystem::path& filename,
            std::ios_base::openmode mode)
    : dyad_basic_fstream<CharT, Traits>(ctx, filename.c_str(), mode)
    {}

#endif /* 201703L */

    template <class CharT, class Traits>
    dyad_basic_fstream<CharT, Traits>::dyad_basic_fstream(
            dyad_basic_fstream<CharT, Traits> &&other)
    : m_buf(std::move(other.m_buf))
    , std::basic_iostream<CharT, Traits>(std::move(other))
    {
        std::basic_iostream<CharT, Traits>::set_rdbuf(&m_buf);
    }
    
    template <class CharT, class Traits>
    dyad_basic_fstream<CharT, Traits>& dyad_basic_fstream<CharT, Traits>::operator=(dyad_basic_fstream<CharT, Traits>&& other)
    {
        m_buf = std::move(other.m_buf);
        std::basic_iostream<CharT, Traits>::operator=(std::move(other));
        return *this;
    }

    template <class CharT, class Traits>
    void dyad_basic_fstream<CharT, Traits>::swap(
            dyad_basic_fstream<CharT, Traits>& rhs)
    {
        m_buf.swap(rhs.m_buf);
        std::basic_iostream<CharT, Traits>::swap(rhs);
    }

    template <class CharT, class Traits>
    bool dyad_basic_fstream<CharT, Traits>::is_open() const
    {
        return rdbuf()->is_open();
    }

#endif /* 201103L */

#if __cplusplus < 201103L

    template <class CharT, class Traits>
    bool dyad_basic_fstream<CharT, Traits>::is_open()
    {
        return rdbuf()->is_open();
    }

#endif

    template <class CharT, class Traits>
    dyad_filebuf<CharT, Traits>* dyad_basic_fstream<CharT, Traits>::rdbuf() const
    {
        return const_cast<dyad_filebuf<CharT, Traits>*>(&m_buf);
    }

    template <class CharT, class Traits>
    void dyad_basic_fstream<CharT, Traits>::open(
            const char* filename,
            std::ios_base::openmode mode)
    {
        dyad_filebuf<CharT, Traits>* ret_ptr = rdbuf()->open(filename, mode);
        if (ret_ptr != NULL)
        {
            std::basic_iostream<CharT, Traits>::clear();
        }
        else 
        {
            std::basic_iostream<CharT, Traits>::setstate(std::ios_base::failbit);
        }
    }

#if __cplusplus >= 201103L

    template <class CharT, class Traits>
    void dyad_basic_fstream<CharT, Traits>::open(
            const std::string& filename,
            std::ios_base::openmode mode)
    {
        open(filename.c_str(), mode);
    }

#if (__cplusplus >= 201703L) && (__has_include(<filesystem>))

    template <class CharT, class Traits>
    template <typename OpenCharType>
    typename std::enable_if<!std::is_same<OpenCharType, char>::value, void>::type 
    dyad_basic_fstream<CharT, Traits>::open(
            const std::filesystem::path::value_type* filename,
            std::ios_base::openmode mode)
    {
        dyad_filebuf<CharT, Traits>* ret_ptr = rdbuf()->open(filename, mode);
        if (ret_ptr != NULL)
        {
            std::basic_iostream<CharT, Traits>::clear();
        }
        else 
        {
            std::basic_iostream<CharT, Traits>::setstate(std::ios_base::failbit);
        }
    }

    template <class CharT, class Traits>
    void dyad_basic_fstream<CharT, Traits>::open(
            const std::filesystem::path& filename,
            std::ios_base::openmode mode)
    {
        open(filename.c_str(), mode);
    }

#endif /* 201703L and <filesystem> */
#endif /* 201103L */

    template <class CharT, class Traits>
    void dyad_basic_fstream<CharT, Traits>::close()
    {
        dyad_filebuf<CharT, Traits>* ret_ptr = rdbuf()->close();
        if (ret_ptr == NULL)
        {
            std::basic_iostream<CharT, Traits>::setstate(std::ios_base::failbit);
        }
    }

    template <class CharT, class Traits>
    void dyad_basic_fstream<CharT, Traits>::init()
    {
        m_buf.init();
    }

    template <class CharT, class Traits>
    void dyad_basic_fstream<CharT, Traits>::init(const dyad_params& p)
    {
        m_buf.init(p);
    }

    template <class CharT, class Traits>
    dyad_stream_core* dyad_basic_fstream<CharT, Traits>::move_stream_core()
    {
        return m_buf.move_stream_core();
    }

    typedef dyad_basic_fstream<char> dyad_fstream;
    typedef dyad_basic_fstream<wchar_t> dyad_wfstream;

}

#endif /* DYAD_FSTREAM_HPP */
