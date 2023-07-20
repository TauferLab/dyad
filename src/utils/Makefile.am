SUBDIRS = libtap

noinst_LTLIBRARIES = libutils.la libmurmur3.la libbase64.la
libutils_la_SOURCES = utils.c utils.h read_all.c read_all.h
libutils_la_CPPFLAGS = \
    $(AM_CPPFLAGS) \
	$(FLUX_CORE_CFLAGS)
libmurmur3_la_SOURCES = murmur3.c murmur3.h
libbase64_la_SOURCES = base64/base64.c base64/base64.h