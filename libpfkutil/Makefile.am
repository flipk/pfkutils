
lib_LTLIBRARIES = libpfkutil.la

libpfkutil_ladir = ${includedir}/util
libpfkutil_la_HEADERS = \
	pk-md5.h types.H bst.H m.h fd_mgr.H base64.h inet_aton.h \
	thread_slinger.h  shmempipe.H  HSM.h  dll3.h  hsmthread.h \
	LockWait.h  prioWorkQ.h  regex.h
libpfkutil_la_SOURCES = \
	pk-md5.c               m.c fd_mgr.C base64.c inet_aton.c \
	thread_slinger.cc shmempipe.cc HSM.cc dll3.cc hsmthread.cc \
	LockWait.cc prioWorkQ.cc regex.c

noinst_PROGRAMS = \
	shmempipe_test_master \
	shmempipe_test_slave \
	thread_slinger_test \
	HSM_test \
	dll3_test \
	hsmthread_test

shmempipe_test_master_SOURCES = shmempipe_test_master.cc
shmempipe_test_master_LDADD   = libpfkutil.la

shmempipe_test_slave_SOURCES = shmempipe_test_slave.cc
shmempipe_test_slave_LDADD   = libpfkutil.la

thread_slinger_test_SOURCES = thread_slinger_test.cc
thread_slinger_test_LDADD   = libpfkutil.la

HSM_test_SOURCES = HSM_test.cc
HSM_test_LDADD   = libpfkutil.la

dll3_test_SOURCES = dll3_test.cc
dll3_test_LDADD   = libpfkutil.la

hsmthread_test_SOURCES = hsmthread_test.cc
hsmthread_test_LDADD   = libpfkutil.la

if PFK_NEED_REGEX
libpfkutil_la_HEADERS += regex.h
libpfkutil_la_SOURCES += regex.c
endif

AM_CPPFLAGS = -I$(top_srcdir)/libpfkdll2

htmldestdir = $(DESTDIR)$(datarootdir)/html/utils

all-local: $(abs_builddir)/doxy_timestamp

$(abs_builddir)/doxy_timestamp: $(libpfkutil_la_HEADERS) $(libpfkutil_la_SOURCES)
if PFK_AM_BUILD_doxygen
	cd $(abs_srcdir) && doxygen $(abs_builddir)/Doxyfile
endif
	touch $(abs_builddir)/doxy_timestamp

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
