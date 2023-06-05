#ifndef DYAD_STREAM_FILEBUF_HPP
#define DYAD_STREAM_FILEBUF_HPP

#include <fstream>
#include <memory>

#inlcude "dyad_core.h"
#include "dyad_params.hpp"

template <
    class CharT,
    class Traits = std::char_traits<CharT>
> class basic_dyad_filebuf : public std::basic_filebuf<CharT, Traits>
{
    public:

        basic_dyad_filebuf();

        basic_dyad_filebuf(const dyad_params& p);

        basic_dyad_filebuf(basic_dyad_filebuf&& rhs);

        basic_dyad_filebuf open(const char* s, std::ios_base::openmode mode);

        basic_dyad_filebuf open(std::string& s, std::ios_base::openmode mode);

        basic_dyad_filebuf open(const std::filesystem

    protected:

#if __cplusplus < 201103L
        dyad_ctx_t* m_dyad_ctx;
#else
        std::unique_ptr<dyad_ctx_t> m_dyad_ctx;
#endif

};

#endif /* DYAD_STREAM_FILEBUF_HPP */
