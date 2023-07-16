noinst_LTLIBRARIES = libstorage_graph.la
libstorage_graph_la_SOURCES = \
    storage_view.c \
    storage_view.h \
    ingesters/linux_mounts_ingester.c \
    ingesters/linux_mounts_ingester.h \
	ingesters/ingester.c \
	ingesters/ingester.h \
    ingester_methods.h \
	storage_check.c \
	storage_check.h
libstorage_graph_la_LIBADD = \
    $(top_builddir)/src/utils/libutils.la \
    $(JANSSON_LIBS)
libstorage_graph_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
	-I$(top_srcdir)/src/utils \
	-I$(top_srcdir)/src/storage_graph/ingesters \
    $(JANSSON_CFLAGS)