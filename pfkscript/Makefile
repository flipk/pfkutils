
PROG_TARGETS = pfkscript

pfkscript_TARGET = $(OBJDIR)/pfkscript
pfkscript_CXXSRCS = logfile.cc options.cc main.cc
pfkscript_INCS = -I../libpfkutil
libpfkutil != find .. -name libpfkutil.a
pfkscript_LIBS= $(libpfkutil) -lpthread -lutil
pfkscript_CXXFLAGS = -Dpfkscript_main=main

include ../Makefile.inc
