
# nobase_include_HEADERS = dll2.H dll2_btree.H

lib_LTLIBRARIES = libpfkdll2.la

libpfkdll2_ladir = ${includedir}/dll2
libpfkdll2_la_HEADERS = dll2.H dll2_btree.H dll2_c.h
libpfkdll2_la_SOURCES = dll2_hash.C

noinst_PROGRAMS = test_btree test_hash

test_btree_SOURCES = test_btree.C
test_hash_SOURCES = test_hash.C test_hash_add.C test_hash_del.C
test_hash_LDADD = libpfkdll2.la
