lib_LTLIBRARIES = libdyad_sync.la
libdyad_sync_la_SOURCES = wrapper.c wrapper.h
libdyad_sync_la_LIBADD = $(top_builddir)/src/core/libdyad_core.la
libdyad_sync_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_builddir)/src/utils \
	-I$(top_builddir)/src/core
libdyad_sync_la_LDFLAGS = \
    $(AM_LDFLAGS) \
	-module \
	-avoid-version \
	-no-undefined \
	-shared
if PERFFLOW
libdyad_sync_la_LIBADD += $(PERFFLOW_LIBS)
libdyad_sync_la_CPPFLAGS += $(PERFFLOW_PLUGIN_CPPFLAGS) $(PERFFLOW_CFLAGS) -DDYAD_PERFFLOW=1
endif

bin_PROGRAMS = flux_barrier
flux_barrier_SOURCES = flux_barrier.c
flux_barrier_LDADD = $(FLUX_CORE_LIBS)
flux_barrier_CPPFLAGS = $(AM_CPPFLAGS) $(FLUX_CORE_CFLAGS)

# TODO uncomment if we decide we want to remove all .la files from install
# install-exec-hook:
# 	@(cd $(DESTDIR)$(libdir) && $(RM) $(lib_LTLIBRARIES)
