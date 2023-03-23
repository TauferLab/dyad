noinst_LTLIBRARIES = libdyad_dtl.la
libdyad_dtl_la_SOURCES = \
	dyad_dtl.c \
	dyad_dtl.h \
	flux_dtl.c \
	flux_dtl.h \
	ucx_dtl.c \
	ucx_dtl.h
libdyad_dtl_la_LIBADD = \
	$(FLUX_CORE_LIBS) \
	$(JANSSON_LIBS) \
	$(UCX_LIBS)
libdyad_dtl_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_builddir)/src/core \
	-I$(top_builddir)/src/utils \
	-I$(top_builddir)/src/utils/base64 \
	$(FLUX_CORE_CFLAGS) \
	$(JANSSON_CFLAGS) \
	$(UCX_CFLAGS)
if PERFFLOW
libdyad_dtl_la_LIBADD += $(PERFFLOW_LIBS)
libdyad_dtl_la_CPPFLAGS += \
	$(PERFFLOW_PLUGIN_CPPFLAGS) \
	$(PERFFLOW_CFLAGS) \
	-DDYAD_PERFFLOW=1
endif
