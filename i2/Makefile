
OBJDIR = obj

PROG_TARGETS = i2 i2f

i2_TARGET = $(OBJDIR)/i2
i2_CXXSRCS = i2_options.cc i2_main.cc
i2_INCS = -I../libpfkutil
i2_DEFS = -Di2_main=main
i2_LIBS = -lpthread

i2f_TARGET = $(OBJDIR)/i2f
i2f_CXXSRCS = i2f_options.cc i2f_main.cc
i2f_INCS = -I../libpfkutil
i2f_DEFS = -Di2f_main=main
i2f_LIBS = -lpthread

include ../Makefile.inc
