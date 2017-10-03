
lib_LTLIBRARIES = libpfkfb.la

libpfkfb_ladir = ${includedir}/fb
libpfkfb_la_HEADERS = \
	BlockCache.H Btree.H BtreeDbClasses.H FileBlockLocal.H \
	FileBlock_iface.H PageCache.H PageIO.H
libpfkfb_la_SOURCES = \
	BlockCache.C Btree.C BtreeNode.C \
	FileBlockLocal.C FileBlockLocalAllocFree.C \
	FileBlockLocalAuid.C FileBlockLocalAun.C FileBlockLocalBucket.C \
	FileBlockLocalCompact.C FileBlockLocalDIB.C FileBlockLocalGetRel.C \
	FileBlockLocalValidate.C FileBlock_iface.C \
	PageCache.C PageIO.C PageIOFileDescriptor.C PageIONetworkTCPServer.C \
	PageCache_internal.H BlockCache_internal.H Btree_internal.H

AM_CPPFLAGS = -I$(top_srcdir)/libpfkdll2 -I$(top_srcdir)/libpfkutil

noinst_PROGRAMS = testBtree testFileBlock

testBtree_SOURCES = testBtree.C
testBtree_LDADD = \
	libpfkfb.la \
	../libpfkdll2/libpfkdll2.la \
	../libpfkutil/libpfkutil.la
testFileBlock_SOURCES = testFileBlock.C
testFileBlock_LDADD = \
	libpfkfb.la \
	../libpfkdll2/libpfkdll2.la \
	../libpfkutil/libpfkutil.la

htmldestdir = $(DESTDIR)$(datarootdir)/html/fb

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
	cp $(abs_builddir)/libpfkfb.tag $(datarootdir)
endif
