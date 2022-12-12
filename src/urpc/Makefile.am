lib_LTLIBRARIES = liburpc_client.la urpc.la

liburpc_client_la_SOURCES = \
    urpc_client.c \
	urpc_client.h
liburpc_client_la_LIBADD = \
	$(FLUX_CORE_LIBS) \
	$(top_builddir)/src/utils/libutils.la
liburpc_client_la_CPPFLAGS = \
	$(FLUX_CORE_CFLAGS) \
	-DURPC_CHECK=1 \
	-I$(top_builddir)/src/utils
if PERFFLOW
liburpc_client_la_LIBADD += $(PERFFLOW_LIBS)
liburpc_client_la_CPPFLAGS += \
    $(PERFFLOW_PLUGIN_CPPFLAGS) \
	$(PERFFLOW_CFLAGS) \
	-DDYAD_PERFFLOW=1
endif

urpc_la_SOURCES = \
	urpc.c \
	urpc_server_ctx.h
urpc_la_LDFLAGS = \
	-module \
	-avoid-version \
	-no-undefined
urpc_la_LIBADD = \
	$(top_builddir)/src/utils/libutils.la \
	$(FLUX_CORE_LIBS) \
	$(JANSSON_LIBS)
urpc_la_CPPFLAGS = \
	$(FLUX_CORE_CFLAGS) \
	$(JANSSON_CFLAGS) \
	-I$(top_builddir)/src/utils
if PERFFLOW
urpc_la_LIBADD = $(PERFFLOW_LIBS)
urpc_la_CPPFLAGS = \
	$(PERFFLOW_PLUGIN_CPPFLAGS) \
	$(PERFFLOW_CFLAGS) \
	-DDYAD_PERFFLOW=1
endif
