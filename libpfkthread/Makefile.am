
lib_LTLIBRARIES = libpfkthread.la

libpfkthread_ladir = ${includedir}/thread


libpfkthread_la_HEADERS = \
	h/pk_filedescs.h \
	h/pk_messages.h \
	h/pk_messages_ext.h \
	h/pk_semaphores.h \
	h/pk_threads.h \
	h/pk_timers.h \
	h/stringhash.h

libpfkthread_la_SOURCES = \
	pk_filedescs.cc pk_filedescs_thread.cc \
	pk_messages.cc pk_messages_ext.cc pk_messages_ext_link_tcp.cc \
	pk_semaphores.cc pk_threads.cc pk_timers.cc stringhash.cc \
	int/pk_filedescs_internal.h \
	int/pk_messages_internal.h \
	int/pk_threads_internal.h \
	int/pk_timers_internal.h

AM_CPPFLAGS = \
	-I$(srcdir)/h \
	-I$(srcdir)/int \
	-I$(top_srcdir)/libpfkdll2 \
	-I$(top_srcdir)/libpfkutil

noinst_PROGRAMS = test-1 test-2 test-3 test-4

test_1_SOURCES = test-1.cc
test_1_LDADD = \
	libpfkthread.la ../libpfkdll2/libpfkdll2.la ../libpfkutil/libpfkutil.la
test_2_SOURCES = test-2.cc
test_2_LDADD = \
	libpfkthread.la ../libpfkdll2/libpfkdll2.la ../libpfkutil/libpfkutil.la
test_3_SOURCES = test-3.cc
test_3_LDADD = \
	libpfkthread.la ../libpfkdll2/libpfkdll2.la ../libpfkutil/libpfkutil.la
test_4_SOURCES = test-4.cc
test_4_LDADD = \
	libpfkthread.la ../libpfkdll2/libpfkdll2.la ../libpfkutil/libpfkutil.la

htmldestdir = $(DESTDIR)$(datarootdir)/html/thread

all-local: $(abs_builddir)/doxy_timestamp

$(abs_builddir)/doxy_timestamp: $(libpfkthread_la_HEADERS) $(libpfkthread_la_SOURCES)
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
	cp $(abs_builddir)/libpfkthread.tag $(datarootdir)
endif
