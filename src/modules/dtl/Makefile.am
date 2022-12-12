noinst_LTLIBRARIES = libdyad_mod_dtl.la

libdyad_mod_dtl_la_SOURCES = \
    dyad_mod_dtl.c \
	dyad_mod_dtl.h \
	flux_mod_dtl.c \
	flux_mod_dtl.h \
	ucx_mod_dtl.c \
	ucx_mod_dtl.h

libdyad_mod_dtl_la_LIBADD = \
	$(FLUX_CORE_LIBS) \
	$(UCX_LIBS)

libdyad_mod_dtl_la_CPPFLAGS = \
	$(FLUX_CORE_CFLAGS) \
	-I$(top_builddir)/src/modules \
	$(UCX_CFLAGS)

if PERFFLOW
libdyad_mod_dtl_la_LIBADD += $(PERFFLOW_LIBS)
libdyad_mod_dtl_la_CPPFLAGS += \
    $(PERFFLOW_PLUGIN_CPPFLAGS) \
	$(PERFFLOW_CFLAGS) \
	-DDYAD_PERFFLOW=1
endif
