noinst_LTLIBRARIES = libdyad_dtl.la
libdyad_dtl_la_SOURCES = \
    dyad_dtl.c \
	dyad_dtl.h \
	dyad_dtl_defs.h \
	flux_dtl.c \
	flux_dtl.h \
	ucx_dtl.c \
	ucx_dtl.h
libdyad_dtl_la_LIBADD = $(FLUX_CORE_LIBS)
libdyad_dtl_la_CPPFLAGS = \
    $(FLUX_CORE_CFLAGS) \
	-I$(top_builddir)/src/core
if UCX
libdyad_dtl_la_LIBADD += $(UCX_LIBS)
libdyad_dtl_la_CPPFLAGS += \
    $(UCX_CFLAGS) \
	-DDYAD_DTL_UCX_ENABLE=1
endif
if PERFFLOW
libdyad_dtl_la_LIBADD += $(PERFFLOW_LIBS)
libdyad_dtl_la_CPPFLAGS += \
    $(PERFFLOW_PLUGIN_CPPFLAGS) \
	$(PERFFLOW_CFLAGS) \
	-DDYAD_PERFFLOW=1
endif
