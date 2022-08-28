# -*- Mode:makefile-gmake; tab-width:8 -*-

LIB_TARGETS = libpfkscriptutil

libpfkscriptutil_TARGET = $(OBJDIR)/libpfkscriptutil.a
libpfkscriptutil_CXXSRCS = libpfkscriptutil.cc
libpfkscriptutil_INCS = -I../libpfkutil

LIB_TARGETS += mini_libpfkutil

mini_libpfkutil_TARGET = $(OBJDIR)/libminipfkutil.a
mini_libpfkutil_CXXSRCS = ../libpfkutil/childprocessmanager.cc

PROG_TARGETS = pfkscript

pfkscript_TARGET = $(OBJDIR)/pfkscript
pfkscript_CXXSRCS = logfile.cc options.cc main.cc
pfkscript_LIBS = -lpthread -lutil
pfkscript_INCS = -I../libpfkutil
pfkscript_CXXFLAGS = -Dpfkscript_main=main
pfkscript_DEPLIBS = $(libpfkscriptutil_TARGET) $(mini_libpfkutil_TARGET)

include ../Makefile.inc
