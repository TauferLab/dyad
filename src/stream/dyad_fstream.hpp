#ifndef DYAD_FSTREAM_HPP
#define DYAD_FSTREAM_HPP

#include "dyad_fstream_core.hpp"

#include <fstream>
#include <string>
#include <type_traits>
#include <utility>

namespace dyad
{

#if (__cplusplus >= 201703L) && __has_include(<filesystem>)
template <typename _Path,
          typename _Result = _Path,
          typename _Path2 =
              decltype (std::declval<_Path&> ().make_preferred ().filename ())>
using is_fs_path =
    std::enable_if_t<std::is_same_v<_Path, _Path2>, _Result>;
#endif

template <typename _CharT, typename _Traits=std::char_traits<_CharT>>
class basic_ifstream : public std::basic_ifstream<_CharT, _Traits>
{
    public:

        basic_ifstream ();
        basic_ifstream (const char *filename, std::ios_base::openmode mode=std::ios_base::in);
#if __cplusplus >= 201103L
        basic_ifstream (const std::string &filename, std::ios_base::openmode mode=std::ios_base::in);
        basic_ifstream (basic_ifstream &&other);
        basic_ifstream (const basic_ifstream& rhs) = delete;
#endif
#if (__cplusplus >= 201703L) && __has_include(<filesystem>)
        template <typename _FsPath, typename _Required=is_fs_path<FsPath>>
        basic_ifstream (const _FsPath &filepath, std::ios_base::openmode mode=std::ios_base::in);
#endif

        virtual ~basic_ifstream ();

        void open (const char *filename, std::ios_base::openmode mode=std::ios_base::in);
#if __cplusplus >= 201103L
        void open (const std::string &filename, std::ios_base::openmode mode=std::ios_base::in);
#endif
#if (__cplusplus >= 201703L) && __has_include(<filesystem>)
        template <typename _FsPath, typename _Required=is_fs_path<FsPath>>
        void open (const _FsPath &filename, std::ios_base::openmode mode=std::ios_base::in);
#endif

        void close ();

    private:

        dyad_fstream_core m_core;
        std::string m_filename;

};

using ifstream = basic_ifstream<char>;
using wifstream = basic_ifstream<wchar_t>;

template <typename _CharT, typename _Traits=std::char_traits<_CharT>>
basic_ifstream<_CharT, _Traits>::basic_ifstream ()
{
}

template <typename _CharT, typename _Traits=std::char_traits<_CharT>>
basic_ifstream<_CharT, _Traits>::basic_ifstream (const char *filename, std::ios_base::openmode mode);

#if __cplusplus >= 201103L

template <typename _CharT, typename _Traits=std::char_traits<_CharT>>
basic_ifstream<_CharT, _Traits>::basic_ifstream (const std::string &filename, std::ios_base::openmode mode=std::ios_base::in);

template <typename _CharT, typename _Traits=std::char_traits<_CharT>>
basic_ifstream<_CharT, _Traits>::basic_ifstream (basic_ifstream &&other);

template <typename _CharT, typename _Traits=std::char_traits<_CharT>>
basic_ifstream<_CharT, _Traits>::basic_ifstream (const basic_ifstream& rhs) = delete;

#endif

#if (__cplusplus >= 201703L) && __has_include(<filesystem>)

template <typename _CharT, typename _Traits=std::char_traits<_CharT>>
template <typename _FsPath, typename _Required=is_fs_path<FsPath>>
basic_ifstream<_CharT, _Traits>::basic_ifstream (const _FsPath &filepath, std::ios_base::openmode mode=std::ios_base::in);

#endif

template <typename _CharT, typename _Traits=std::char_traits<_CharT>>
basic_ifstream<_CharT, _Traits>::~basic_ifstream ();

template <typename _CharT, typename _Traits=std::char_traits<_CharT>>
void basic_ifstream<_CharT, _Traits>::open (const char *filename, std::ios_base::openmode mode=std::ios_base::in);

#if __cplusplus >= 201103L

template <typename _CharT, typename _Traits=std::char_traits<_CharT>>
void basic_ifstream<_CharT, _Traits>::open (const std::string &filename, std::ios_base::openmode mode=std::ios_base::in);

#endif

#if (__cplusplus >= 201703L) && __has_include(<filesystem>)

template <typename _CharT, typename _Traits=std::char_traits<_CharT>>
template <typename _FsPath, typename _Required=is_fs_path<FsPath>>
void basic_ifstream<_CharT, _Traits>::open (const _FsPath &filename, std::ios_base::openmode mode=std::ios_base::in);

#endif

template <typename _CharT, typename _Traits=std::char_traits<_CharT>>
void basic_ifstream<_CharT, _Traits>::close ();

}

#endif /* DYAD_FSTREAM_HPP */
