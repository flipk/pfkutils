
# nobase_include_HEADERS = dll2.h dll2_btree.h

lib_LTLIBRARIES = libpfkdll2.la

libpfkdll2_ladir = ${includedir}/dll2
libpfkdll2_la_HEADERS = dll2.h dll2_btree.h dll2_c.h
libpfkdll2_la_SOURCES = dll2_hash.cc

noinst_PROGRAMS = test_btree test_hash

test_btree_SOURCES = test_btree.cc
test_hash_SOURCES = test_hash.cc test_hash_add.cc test_hash_del.cc
test_hash_LDADD = libpfkdll2.la

htmldestdir = $(DESTDIR)$(datarootdir)/html/dll2

all-local: $(abs_builddir)/doxy_timestamp

$(abs_builddir)/doxy_timestamp: $(libpfkdll2_la_HEADERS) $(libpfkdll2_la_SOURCES)
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
	cp $(abs_builddir)/libpfkdll2.tag $(datarootdir)
endif
