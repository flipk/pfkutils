
OBJDIR = obj

PROG_TARGETS += m
m_TARGET = $(OBJDIR)/m
m_CSRCS = m.c
m_DEFS = -Dm_main=main

PROG_TARGETS += base64_test
base64_test_TARGET = $(OBJDIR)/base64_test
base64_test_CXXSRCS = base64.cc
base64_test_DEFS = -DINCLUDE_BASE64_TEST_MAIN

PROG_TARGETS += printbuf_test
printbuf_test_TARGET = $(OBJDIR)/libpfkutil-printbuf_test
printbuf_test_CSRCS = printbuf.c

LIB_TARGETS += libpfkutil

libpfkutil_TARGET = $(OBJDIR)/libpfkutil.a
libpfkutil_CXXSRCS = \
	hsmthread.cc signal_backtrace.cc \
	dll3.cc dll3_btree.cc \
	childprocessmanager.cc \
	thread_slinger.cc \
	newbase64.cc

LIBPFKUTIL_TEST_TARGETS = \
        hsmthread_test dll3_test dll3_btree_test \
        childprocessmanager_test signal_backtrace_test \
        thread_slinger_test posix_fe_test newbase64_test

define BUILD_LIBPFKUTIL_TEST_TARGET
PROG_TARGETS += $(t)
$(t)_TARGET = $(OBJDIR)/libpfkutil-$(t)
$(t)_CXXSRCS = $(t).cc
$(t)_LIBS += -lpthread -lrt
$(t)_DEPLIBS += $(libpfkutil_TARGET)

endef

$(eval $(foreach t,$(LIBPFKUTIL_TEST_TARGETS),$(BUILD_LIBPFKUTIL_TEST_TARGET)))

include ../Makefile.inc
