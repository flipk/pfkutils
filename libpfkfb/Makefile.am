
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
