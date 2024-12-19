
OBJDIR= obj
LIB_TARGETS =
PROG_TARGETS = 

LIB_TARGETS += tslib
tslib_TARGET = $(OBJDIR)/thread_states.a
tslib_CXXSRCS = thread_states.cc

PROG_TARGETS += tstest
tstest_TARGET = $(OBJDIR)/thread_states_test
tstest_CXXSRCS = thread_states_test.cc
tstest_DEPLIBS = $(tslib_TARGET)

PROG_TARGETS += tstprint
tstprint_TARGET = $(OBJDIR)/thread_states_print
tstprint_CXXSRCS = thread_states_print.cc
tstprint_DEPLIBS = $(tslib_TARGET)

include ../../Makefile.inc
