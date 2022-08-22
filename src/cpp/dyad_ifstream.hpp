#ifndef DYAD_IFSTREAM_HPP
#define DYAD_IFSTREAM_HPP

#include "dyad_stream_buf.hpp"
#include "dyad_stream_core.hpp"
#include <istream>

namespace dyad {

    template <class CharT, class Traits = std::char_traits<CharT>>
    class dyad_basic_ifstream : public std::basic_istream<CharT, Traits> {
        public:

            dyad_basic_ifstream();
            dyad_basic_ifstream(dyad_stream_core *ctx);
            dyad_basic_ifstream(const char *filename,
                    std::ios_base::openmode mode=std::ios_base::in);
            dyad_basic_ifstream(dyad_stream_core *ctx,
                    const char *filename,
                    std::ios_base::openmode mode=std::ios_base::in);
#if __cplusplus >= 201103L
            dyad_basic_ifstream(const std::string& filename,
                    std::ios_base::openmode mode=std::ios_base::in);
            dyad_basic_ifstream(dyad_stream_core *ctx,
                    const std::string& filename,
                    std::ios_base::openmode mode=std::ios_base::in);
            dyad_basic_ifstream(dyad_basic_ifstream<CharT, Traits>&& other);
            dyad_basic_ifstream(const dyad_basic_ifstream<CharT, Traits>& other) = delete;
#if (__cplusplus >= 201703L) && __has_include(<filesystem>)
            template <typename OpenCharType = std::filesystem::path::value_type,
                      typename = std::enable_if<!std::is_same<OpenCharType, char>::value, void>>
            dyad_basic_ifstream(const std::filesystem::path::value_type* filename,
                    std::ios_base::openmode mode=std::ios_base::in);
            template <typename OpenCharType = std::filesystem::path::value_type,
                      typename = std::enable_if<!std::is_same<OpenCharType, char>::value, void>>
            dyad_basic_ifstream(dyad_stream_core *ctx,
                    const std::filesystem::path::value_type* filename,
                    std::ios_base::openmode mode=std::ios_base::in);
            dyad_basic_ifstream(const std::filesystem::path& filename,
                    std::ios_base::openmode mode=std::ios_base::in);
            dyad_basic_ifstream(dyad_stream_core *ctx,
                    const std::filesystem::path& filename,
                    std::ios_base::openmode mode=std::ios_base::in);
#endif /* 201703L and <filesystem> */

            dyad_basic_ifstream<CharT, Traits>& operator=(dyad_basic_ifstream<CharT, Traits>&& other);
            
            void swap(dyad_basic_ifstream<CharT, Traits>& other);

            bool is_open() const;
#endif /* 201103L */
#if __cplusplus < 201103L
            bool is_open();
#endif

            dyad_filebuf<CharT, Traits>* rdbuf() const;

            void open(const char* filename,
                    std::ios_base::openmode mode=std::ios_base::in);
#if __cplusplus >= 201103L
            void open(const std::string& filename,
                    std::ios_base::openmode mode=std::ios_base::in);
#if (__cplusplus >= 201703L) && __has_include(<filesystem>)
            template <typename OpenCharType = std::filesystem::path::value_type>
            typename std::enable_if<!std::is_same<OpenCharType, char>::value, void>::type 
                open(const std::filesystem::path::value_type *filename,
                    std::ios_base::openmode mode=std::ios_base::in);
            void open(const std::filesystem::path& filename,
                    std::ios_base::openmode mode=std::ios_base::in);
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
    template <class CharT, class Traits = std::char_traits<CharT>>   
    void swap(dyad_basic_ifstream<CharT, Traits>& x,
            dyad_basic_ifstream<CharT, Traits>& y)
    {
        x.swap(y);
    }
#endif
                                           
    template <class CharT, class Traits>   
    dyad_basic_ifstream<CharT, Traits>::dyad_basic_ifstream()
        : m_buf()                          
        , std::basic_istream<CharT, Traits>(&m_buf)
    {}                                     

    template <class CharT, class Traits>
    dyad_basic_ifstream<CharT, Traits>::dyad_basic_ifstream(dyad_stream_core *ctx)
        : m_buf(ctx)
        , std::basic_istream<CharT, Traits>(&m_buf)
    {}

    template <class CharT, class Traits>
    dyad_basic_ifstream<CharT, Traits>::dyad_basic_ifstream(
            const char *filename,
            std::ios_base::openmode mode)
        : dyad_basic_ifstream<CharT, Traits>()
    {
        dyad_filebuf<CharT, Traits>* ret_ptr = rdbuf()->open(filename,
                mode | std::ios_base::in);
        if (ret_ptr == NULL)
        {
            std::basic_istream<CharT, Traits>::setstate(std::ios_base::failbit);
        }
    }

    template <class CharT, class Traits>
    dyad_basic_ifstream<CharT, Traits>::dyad_basic_ifstream(dyad_stream_core *ctx,
            const char *filename,
            std::ios_base::openmode mode)
        : dyad_basic_ifstream<CharT, Traits>(ctx)
    {
        dyad_filebuf<CharT, Traits>* ret_ptr = rdbuf()->open(filename,
                mode | std::ios_base::in);
        if (ret_ptr == NULL)
        {
            std::basic_istream<CharT, Traits>::setstate(std::ios_base::failbit);
        }
    }

#if __cplusplus >= 201103L

    template <class CharT, class Traits>
    dyad_basic_ifstream<CharT, Traits>::dyad_basic_ifstream(
            const std::string& filename,
            std::ios_base::openmode mode)
        : dyad_basic_ifstream<CharT, Traits>(filename.c_str(), mode)
    {}

    template <class CharT, class Traits>
    dyad_basic_ifstream<CharT, Traits>::dyad_basic_ifstream(dyad_stream_core *ctx,
            const std::string& filename,
            std::ios_base::openmode mode)
        : dyad_basic_ifstream<CharT, Traits>(ctx, filename.c_str(), mode)
    {}

    template <class CharT, class Traits>
    dyad_basic_ifstream<CharT, Traits>::dyad_basic_ifstream(
            dyad_basic_ifstream<CharT, Traits>&& other)
        : m_buf(std::move(other.m_buf))
        , std::basic_istream<CharT, Traits>(std::move(other))
    {
        std::basic_istream<CharT, Traits>::set_rdbuf(&m_buf);
    }

#if (__cplusplus >= 201703L) && __has_include(<filesystem>)

    template <class CharT, class Traits>
    template <typename OpenCharType, typename>
    dyad_basic_ifstream<CharT, Traits>::dyad_basic_ifstream(
            const std::filesystem::path::value_type* filename,
            std::ios_base::openmode mode)
        : dyad_basic_ifstream<CharT, Traits>()
    {
        dyad_filebuf<CharT, Traits>* ret_ptr = rdbuf()->open(filename,
                mode | std::ios_base::in);
        if (ret_ptr == NULL)
        {
            std::basic_istream<CharT, Traits>::setstate(std::ios_base::failbit);
        }
    }

    template <class CharT, class Traits>
    template <typename OpenCharType, typename>
    dyad_basic_ifstream<CharT, Traits>::dyad_basic_ifstream(dyad_stream_core *ctx,
            const std::filesystem::path::value_type* filename,
            std::ios_base::openmode mode)
        : dyad_basic_ifstream<CharT, Traits>(ctx)
    {
        dyad_filebuf<CharT, Traits>* ret_ptr = rdbuf()->open(filename,
                mode | std::ios_base::in);
        if (ret_ptr == NULL)
        {
            std::basic_istream<CharT, Traits>::setstate(std::ios_base::failbit);
        }
    }

    template <class CharT, class Traits>
    dyad_basic_ifstream<CharT, Traits>::dyad_basic_ifstream(
            const std::filesystem::path& filename,
            std::ios_base::openmode mode)
        : dyad_basic_ifstream<CharT, Traits>(filename.c_str(), mode)
    {}

    template <class CharT, class Traits>
    dyad_basic_ifstream<CharT, Traits>::dyad_basic_ifstream(dyad_stream_core *ctx,
            const std::filesystem::path& filename,
            std::ios_base::openmode mode)
        : dyad_basic_ifstream<CharT, Traits>(ctx, filename.c_str(), mode)
    {}

#endif /* 201703L and <filesystem> */

    template <class CharT, class Traits>
    dyad_basic_ifstream<CharT, Traits>& dyad_basic_ifstream<CharT, Traits>::operator=(dyad_basic_ifstream<CharT, Traits>&& other)
    {
        m_buf = std::move(other.m_buf);
        std::basic_istream<CharT, Traits>::operator=(std::move(other));
        return *this;
    }
    
    template <class CharT, class Traits>
    void dyad_basic_ifstream<CharT, Traits>::swap(dyad_basic_ifstream<CharT, Traits>& other)
    {
        m_buf.swap(other.m_buf);
        std::basic_istream<CharT, Traits>::swap(other);
    }

    template <class CharT, class Traits>
    bool dyad_basic_ifstream<CharT, Traits>::is_open() const
    {
        return rdbuf()->is_open();
    }
#endif /* 201103L */

#if __cplusplus < 201103L
    template <class CharT, class Traits>
    bool dyad_basic_ifstream<CharT, Traits>::is_open()
    {
        return rdbuf()->is_open();
    }
#endif

    template <class CharT, class Traits>
    dyad_filebuf<CharT, Traits>* dyad_basic_ifstream<CharT, Traits>::rdbuf() const
    {
        return const_cast<dyad_filebuf<CharT, Traits>*>(&m_buf);
    }

    template <class CharT, class Traits>
    void dyad_basic_ifstream<CharT, Traits>::open(const char* filename,
            std::ios_base::openmode mode)
    {
        dyad_filebuf<CharT, Traits> ret_buf = rdbuf()->open(filename, mode | std::ios_base::in);
        if (ret_buf != NULL)
        {
            std::basic_istream<CharT, Traits>::clear();
        }
        else
        {
            std::basic_istream<CharT, Traits>::setstate(std::ios_base::failbit);
        }
    }

#if __cplusplus >= 201103L

    template <class CharT, class Traits>
    void dyad_basic_ifstream<CharT, Traits>::open(const std::string& filename,
            std::ios_base::openmode mode)
    {
        open(filename.c_str(), mode);
    }

#if (__cplusplus >= 201703L) && __has_include(<filesystem>)

    template <class CharT, class Traits>
    template <typename OpenCharType>
    typename std::enable_if<!std::is_same<OpenCharType, char>::value, void>::type 
    dyad_basic_ifstream<CharT, Traits>::open(
            const std::filesystem::path::value_type *filename,
            std::ios_base::openmode mode)
    {
        dyad_filebuf<CharT, Traits> ret_buf = rdbuf()->open(filename, mode | std::ios_base::in);
        if (ret_buf != NULL)
        {
            std::basic_istream<CharT, Traits>::clear();
        }
        else
        {
            std::basic_istream<CharT, Traits>::setstate(std::ios_base::failbit);
        }
    }

    template <class CharT, class Traits>
    void dyad_basic_ifstream<CharT, Traits>::open(
            const std::filesystem::path& filename,
            std::ios_base::openmode mode)
    {
        open(filename.c_str(), mode);
    }

#endif /* 201703L and <filesystem> */
#endif /* 201103L */
    
    template <class CharT, class Traits>
    void dyad_basic_ifstream<CharT, Traits>::close()
    {
        dyad_filebuf<CharT, Traits> ret_buf = rdbuf()->close();
        if (ret_buf == NULL)
        {
            std::basic_istream<CharT, Traits>::setstate(std::ios_base::failbit);
        }
    }

    template <class CharT, class Traits>
    void dyad_basic_ifstream<CharT, Traits>::init()
    {
        m_buf.init();
    }

    template <class CharT, class Traits>
    void dyad_basic_ifstream<CharT, Traits>::init(const dyad_params& p)
    {
        m_buf.init(p);
    }

    template <class CharT, class Traits>
    dyad_stream_core* dyad_basic_ifstream<CharT, Traits>::move_stream_core()
    {
        return m_buf.move_stream_core();
    }

    typedef dyad_basic_ifstream<char> dyad_ifstream;
    typedef dyad_basic_ifstream<wchar_t> dyad_wifstream;

}

#endif /* DYAD_IFSTREAM_HPP */
