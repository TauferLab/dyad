lib_LTLIBRARIES = dyad.la
dyad_la_SOURCES = \
    dyad.c \
	dyad_flux_log.h
dyad_la_LDFLAGS = \
	-module \
	-avoid-version \
	-no-undefined
dyad_la_LIBADD = \
	$(top_builddir)/src/utils/libutils.la \
	$(FLUX_CORE_LIBS)
dyad_la_CPPFLAGS = \
	$(FLUX_CORE_CFLAGS) \
	-I$(top_builddir)/src/utils
if PERFFLOW
dyad_la_LIBADD += $(PERFFLOW_LIBS)
dyad_la_CPPFLAGS += \
	$(PERFFLOW_PLUGIN_CPPFLAGS) \
	$(PERFFLOW_CFLAGS) \
	-DDYAD_PERFFLOW=1
endif
