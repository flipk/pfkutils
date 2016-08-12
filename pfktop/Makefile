
PROG_TARGETS= pfktop

pfktop_TARGET = $(OBJDIR)/pfktop
pfktop_CXXSRCS = pfktop.cc \
	pidlist.cc options.cc screen.cc \
	fileparser.cc tiddb.cc tidentry.cc
pfktop_DEFS = -Dpfktop_main=main
pfktop_INCS = -I../libpfkutil

include ../Makefile.inc
