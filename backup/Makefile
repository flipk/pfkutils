
OBJDIR= obj.$(PFKARCH)

PROG_TARGETS += pfkbak
pfkbak_TARGET = $(OBJDIR)/pfkbak
pfkbak_HDRS    = \
	bakfile.h database_items.h options.h
pfkbak_CXXSRCS = \
	bakfile.cc create.cc deletevers.cc export.cc \
	extract.cc list.cc main.cc options.cc update.cc
pfkbak_INCS = \
	-I$(HOME)/pfk/include/libpfkfb \
	-I$(HOME)/pfk/include/libpfkutil \
	-I$(HOME)/pfk/include/libpfkdll2
pfkbak_DEFS = -Dpfkbak_main=main
pfkbak_LIBS = \
	-L$(HOME)/pfk/$(PFKARCH)/lib -lpfkfb -lpfkutil -lpfkdll2

include ../Makefile.inc
