
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

htmldestdir = $(DESTDIR)$(datarootdir)/html/utils

all-local:
if PFK_AM_BUILD_doxygen
	cd $(abs_srcdir) && doxygen $(abs_builddir)/Doxyfile
endif

install-data-local:
if PFK_AM_BUILD_doxygen
	mkdir -p $(htmldestdir) || true
	mkdir -p $(mandir) || true
	cd $(abs_builddir)/dox/html && \
		$(AMTAR) cf - . | ( cd $(htmldestdir) ; $(AMTAR) xf - )
	cd $(abs_builddir)/dox/man && \
		$(AMTAR) cf - . | ( cd $(mandir) ; $(AMTAR) xf - )
	cp $(abs_builddir)/libpfkutil.tag $(datarootdir)
endif
