
lib_LTLIBRARIES = libpfkutil.la

libpfkutil_ladir = ${includedir}/util
libpfkutil_la_HEADERS = \
	pk-md5.h types.H bst.H m.h fd_mgr.H base64.h inet_aton.h
libpfkutil_la_SOURCES = \
	pk-md5.c               m.c fd_mgr.C base64.c inet_aton.c

if PFK_NEED_REGEX
libpfkutil_la_HEADERS += regex.h
libpfkutil_la_SOURCES += regex.c
endif

AM_CPPFLAGS = -I$(top_srcdir)/libpfkdll2
