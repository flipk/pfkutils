# -*- Mode:makefile-gmake; tab-width:8 -*-

LIB_TARGETS = libpfkscriptutil

libpfkscriptutil_TARGET = $(OBJDIR)/libpfkscriptutil.a
libpfkscriptutil_CXXSRCS = libpfkscriptutil.cc
libpfkscriptutil_INCS = -I../libpfkutil

PROG_TARGETS = pfkscript

pfkscript_TARGET = $(OBJDIR)/pfkscript
pfkscript_CXXSRCS = logfile.cc options.cc main.cc
pfkscript_INCS = -I../libpfkutil
libpfkutil = $(shell find .. -name libpfkutil.a)
pfkscript_LIBS= $(libpfkutil) -lpthread -lutil
pfkscript_CXXFLAGS = -Dpfkscript_main=main
pfkscript_DEPLIBS = $(libpfkscriptutil_TARGET)

include ../Makefile.inc
