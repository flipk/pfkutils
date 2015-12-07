
lib_LTLIBRARIES = libpfkfb.la

libpfkfb_ladir = ${includedir}/fb
libpfkfb_la_HEADERS = \
	BlockCache.h Btree.h BtreeDbClasses.h FileBlockLocal.h \
	FileBlock_iface.h PageCache.h PageIO.h
libpfkfb_la_SOURCES = \
	BlockCache.cc Btree.cc BtreeNode.cc \
	FileBlockLocal.cc FileBlockLocalAllocFree.cc \
	FileBlockLocalAuid.cc FileBlockLocalAun.cc FileBlockLocalBucket.cc \
	FileBlockLocalCompact.cc FileBlockLocalDIB.cc FileBlockLocalGetRel.cc \
	FileBlockLocalValidate.cc FileBlock_iface.cc \
	PageCache.cc PageIO.cc PageIOFileDescriptor.cc PageIONetworkTCPServer.cc \
	PageCache_internal.h BlockCache_internal.h Btree_internal.h

AM_CPPFLAGS = -I$(top_srcdir)/libpfkdll2 -I$(top_srcdir)/libpfkutil

noinst_PROGRAMS = testBtree testFileBlock

testBtree_SOURCES = testBtree.cc
testBtree_LDADD = \
	libpfkfb.la \
	../libpfkdll2/libpfkdll2.la \
	../libpfkutil/libpfkutil.la
testFileBlock_SOURCES = testFileBlock.cc
testFileBlock_LDADD = \
	libpfkfb.la \
	../libpfkdll2/libpfkdll2.la \
	../libpfkutil/libpfkutil.la

htmldestdir = $(DESTDIR)$(datarootdir)/html/fb

all-local: $(abs_builddir)/doxy_timestamp

$(abs_builddir)/doxy_timestamp: $(libpfkfb_la_HEADERS) $(libpfkfb_la_SOURCES)
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
	cp $(abs_builddir)/libpfkfb.tag $(datarootdir)
endif
