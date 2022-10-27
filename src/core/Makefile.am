SUBDIRS = dtl

lib_LTLIBRARIES = libdyad_core.la
libdyad_core_la_SOURCES = \
    dyad_core.c \
	dyad_open_dlwrap.h \
	dyad_open_dlwrap.c
libdyad_core_la_LIBADD = \
    $(top_builddir)/src/utils/libutils.la \
	$(top_builddir)/src/utils/libmurmur3.la \
	$(top_builddir)/src/core/dtl/libdyad_dtl.la \
	$(FLUX_CORE_LIBS)
libdyad_core_la_CPPFLAGS = \
    $(FLUX_CORE_CFLAGS) \
	-I$(top_builddir)/src/utils \
	-I$(top_builddir)/src/core/dtl
if PERFFLOW
libdyad_core_la_LIBADD += $(PERFFLOW_LIBS)
libdyad_core_la_CPPFLAGS += \
    $(PERFFLOW_PLUGIN_CPPFLAGS) \
	$(PERFFLOW_CFLAGS) \
	-DDYAD_PERFFLOW=1
endif

include_HEADERS = dyad_core.h dyad_err.h
