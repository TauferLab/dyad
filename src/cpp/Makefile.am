lib_LTLIBRARIES = libdyad_fstream.la

libdyad_fstream_la_SOURCES = dyad_stream_core.cpp
libdyad_fstream_la_LIBADD = $(top_builddir)/src/utils/libutils.la $(top_builddir)/src/core/libdyad_core.la
libdyad_fstream_la_CPPFLAGS = -DDYAD_CHECK=1 -I$(top_builddir)/src/utils -I$(top_builddir)/src/core
if PERFFLOW
libdyad_fstream_la_LIBADD += $(PERFFLOW_LIBS)
libdyad_fstream_la_CPPFLAGS += $(PERFFLOW_PLUGIN_CPPFLAGS) $(PERFFLOW_CFLAGS) -DDYAD_PERFFLOW=1
endif

include_HEADERS = dyad_params.hpp dyad_stream_core.hpp dyad_stream_buf.hpp dyad_ifstream.hpp dyad_ofstream.hpp dyad_fstream.hpp dyad_stream.hpp
