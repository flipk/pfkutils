
OBJDIR = obj

PROG_TARGETS += bounce

bounce_TARGET = $(OBJDIR)/bounce
bounce_CXXSRCS = bounce.cc
bounce_LIBS = -lpthread -lrt
bounce_INCS = -I../../libpfkutil

PROG_TARGETS += inject

inject_TARGET = $(OBJDIR)/inject
inject_CXXSRCS = inject.cc
inject_INCS = -I../../libpfkutil

include ../../Makefile.inc
