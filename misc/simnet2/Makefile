
LIB_TARGETS= mylib

OBJDIRS_TOMAKE = lib

mylib_TARGET = $(OBJDIR)/lib/libmylib.a
mylib_CXXSRCS = \
	lib/LockWait.cc \
	lib/dll3.cc \
	lib/thread_slinger.cc

PROG_TARGETS = simnet2

simnet2_TARGET = $(OBJDIR)/simnet2
simnet2_CXXSRCS = node.cc printer.cc main.cc
simnet2_DEPLIBS = $(mylib_TARGET)
simnet2_INCS = -Ilib
simnet2_LIBS = -lpthread

include Makefile.inc
