
lib_LTLIBRARIES = libpfkthread.la

libpfkthread_ladir = ${includedir}/thread


libpfkthread_la_HEADERS = \
	h/pk_filedescs.H \
	h/pk_messages.H \
	h/pk_messages_ext.H \
	h/pk_semaphores.H \
	h/pk_threads.H \
	h/pk_timers.H \
	h/stringhash.H

libpfkthread_la_SOURCES = \
	pk_filedescs.C pk_filedescs_thread.C \
	pk_messages.C pk_messages_ext.C pk_messages_ext_link_tcp.C \
	pk_semaphores.C pk_threads.C pk_timers.C stringhash.C \
	int/pk_filedescs_internal.H \
	int/pk_messages_internal.H \
	int/pk_threads_internal.H \
	int/pk_timers_internal.H

AM_CPPFLAGS = \
	-I$(srcdir)/h \
	-I$(srcdir)/int \
	-I$(top_srcdir)/libpfkdll2 \
	-I$(top_srcdir)/libpfkutil

noinst_PROGRAMS = test-1 test-2 test-3 test-4

test_1_SOURCES = test-1.C
test_1_LDADD = \
	libpfkthread.la ../libpfkdll2/libpfkdll2.la ../libpfkutil/libpfkutil.la
test_2_SOURCES = test-2.C
test_2_LDADD = \
	libpfkthread.la ../libpfkdll2/libpfkdll2.la ../libpfkutil/libpfkutil.la
test_3_SOURCES = test-3.C
test_3_LDADD = \
	libpfkthread.la ../libpfkdll2/libpfkdll2.la ../libpfkutil/libpfkutil.la
test_4_SOURCES = test-4.C
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
