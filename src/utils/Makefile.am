SUBDIRS = libtap

noinst_LTLIBRARIES = libutils.la libmurmur3.la
libutils_la_SOURCES = utils.c utils.h read_all.c read_all.h flux_b64.h
libutils_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
	$(FLUX_CORE_CFLAGS)
libutils_la_LIBADD = $(FLUX_CORE_LIBS)
libmurmur3_la_SOURCES = murmur3.c murmur3.h
#libmurmur3_la_CPPFLAGS = $(AM_CPPFLAGS)
#libmurmur3_la_LIBADD =
