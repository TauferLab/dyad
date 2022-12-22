SUBDIRS = dtl

lib_LTLIBRARIES = dyad.la
dyad_la_SOURCES = \
    dyad.c \
	dyad_flux_log.h
dyad_la_LDFLAGS = \
	$(AM_LDFLAGS) \
    -module \
	-avoid-version \
	-no-undefined \
	-shared
dyad_la_LIBADD = \
	$(top_builddir)/src/module/dtl/libdyad_mod_dtl.la \
	$(top_builddir)/src/utils/libutils.la \
	$(FLUX_CORE_LIBS)
dyad_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	$(FLUX_CORE_CFLAGS) \
	$(DYAD_DEBUG_CPPFLAGS) \
	-I$(top_builddir)/src/utils \
	-I$(top_builddir)/src/module/dtl
if PERFFLOW
dyad_la_LIBADD += $(PERFFLOW_LIBS)
dyad_la_CPPFLAGS += \
	$(PERFFLOW_PLUGIN_CPPFLAGS) \
	$(PERFFLOW_CFLAGS) \
	-DDYAD_PERFFLOW=1
endif

# TODO consider adding the following hook to remove .la files from install
# install-exec-hook:
# 	@(cd $(DESTDIR)$(libdir) && $(RM) $(lib_LTLIBRARIES))
