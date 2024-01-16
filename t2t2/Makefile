
OBJDIR = obj

CXXFLAGS += -fdiagnostics-color=always -std=c++11

LIB_TARGETS = t2t2
t2t2_TARGET = $(OBJDIR)/libt2t2.a
t2t2_CXXSRCS = thread2thread2.cc hsm.cc
t2t2_INCS = -I../libpfkutil
t2t2_DOXYFILE = Doxy.t2t2
DOXYGEN_TARGETS += t2t2

PROG_TARGETS = t1
t1_TARGET = $(OBJDIR)/t1
t1_CXXSRCS = test/thread2thread2_test.cc
t1_INCS = -I../libpfkutil -I.
t1_DEPLIBS = $(t2t2_TARGET)
t1_LIBS = -lpthread
TESTS = test1_run

PROG_TARGETS += t2
t2_TARGET = $(OBJDIR)/t2
t2_CXXSRCS = test/hsm_test.cc
t2_INCS = -I../libpfkutil -I.
t2_DEPLIBS = $(t2t2_TARGET)
t2_LIBS = -lpthread
TESTS += test2_run

PROG_TARGETS += t3
t3_TARGET = $(OBJDIR)/t3
t3_CXXSRCS = test/func_sig_regex_test.cc
t3_INCS = -I../libpfkutil

EXTRA_CLEAN += testrun_clean
EXTRA_CLEAN += bundle_clean

include ../Makefile.inc

test: all $(TESTS)

test1_run: 0log.t1

0log.t1: $(t1_TARGET)
	$(t1_TARGET) | tee 0log.t1.temp
	@mv 0log.t1.temp 0log.t1

test2_run: 0log.t2

0log.t2: $(t2_TARGET)
	$(t2_TARGET) | tee 0log.t2.temp
	@mv 0log.t2.temp 0log.t2

testrun_clean:
	rm -f 0log*

bundle:
	git bundle create ts2.bundle --all
	git bundle verify ts2.bundle

bundle_clean:
	rm -f ts2.bundle
