noinst_LTLIBRARIES = libdyad_core_deps.la libdyad_core_internal.la
libdyad_core_deps_la_SOURCES =
libdyad_core_deps_la_LIBADD = \
	$(top_builddir)/src/utils/libutils.la \
	$(top_builddir)/src/utils/libmurmur3.la \
	$(top_builddir)/src/utils/libbase64.la \
	$(JANSSON_LIBS) \
	$(top_builddir)/src/storage_graph/libstorage_graph.la \
	$(UCX_LIBS) \
	$(FLUX_CORE_LIBS)
if PERFFLOW
libdyad_core_deps_la_LIBADD += $(PERFFLOW_LIBS)
endif

libdyad_core_internal_la_SOURCES = \
	dtl/dyad_dtl.c \
	dtl/dyad_dtl.h \
	dtl/flux_dtl.c \
	dtl/flux_dtl.h \
	dtl/ucx_dtl.c \
	dtl/ucx_dtl.h \
	dyad_core.c
libdyad_core_internal_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/src/utils \
	-I$(top_srcdir)/src/utils/base64 \
	-I$(top_srcdir)/src/storage_graph \
    $(UCX_CFLAGS) \
    $(JANSSON_CFLAGS) \
	$(FLUX_CORE_CFLAGS)
if PERFFLOW
libdyad_core_internal_la_CPPFLAGS += $(PERFFLOW_PLUGIN_CPPFLAGS) $(PERFFLOW_CFLAGS) -DDYAD_PERFFLOW=1
endif

lib_LTLIBRARIES = libdyad_core.la
libdyad_core_la_SOURCES =
libdyad_core_la_LIBADD = \
	$(top_builddir)/src/core/libdyad_core_deps.la \
	$(top_builddir)/src/core/libdyad_core_internal.la
libdyad_core_la_LDFLAGS = -export-symbols dyad_core.sym -Wl,-rpath,'$(UCX_LIBDIR)' $(AM_LDFLAGS)

include_HEADERS = dyad_core.h dyad_envs.h dyad_flux_log.h dyad_dtl_defs.h
