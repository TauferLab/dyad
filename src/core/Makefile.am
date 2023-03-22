SUBDIRS = dtl

lib_LTLIBRARIES = libdyad_core.la
libdyad_core_la_SOURCES = dyad_core.c
libdyad_core_la_LIBADD = \
	$(top_builddir)/src/core/dtl/libdyad_dtl.la \
	$(top_builddir)/src/utils/libutils.la \
	$(top_builddir)/src/utils/libmurmur3.la \
	$(FLUX_CORE_LIBS)
libdyad_core_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_builddir)/src/utils $(FLUX_CORE_CFLAGS)
libdyad_core_la_LDFLAGS = $(AM_LDFLAGS) -Wl,-rpath,'$(UCX_LIBDIR)'
if PERFFLOW
libdyad_core_la_LIBADD += $(PERFFLOW_LIBS)
libdyad_core_la_CPPFLAGS += $(PERFFLOW_PLUGIN_CPPFLAGS) $(PERFFLOW_CFLAGS) -DDYAD_PERFFLOW=1
endif

include_HEADERS = dyad_core.h dyad_envs.h dyad_rc.h dyad_flux_log.h dyad_dtl_defs.h
