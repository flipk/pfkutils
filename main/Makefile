
OBJDIR = obj

# he.c random-hex.c rmstar_new.cc treescan.cc

PROG_TARGETS = slowlink
slowlink_TARGET = $(OBJDIR)/slowlink
slowlink_CXXSRCS = slowlink.cc ../libpfkutil/thread_slinger.cc
slowlink_DEFS = -Dslowlink_main=main
slowlink_INCS = -I ../libpfkutil
slowlink_LIBS = -lpthread

PROG_TARGETS += genpw
genpw_TARGET = $(OBJDIR)/genpw
genpw_CSRCS = genpw.c
genpw_DEFS = -Dgenpw_main=main

PROG_TARGETS += waitpid
waitpid_TARGET = $(OBJDIR)/waitpid
waitpid_CSRCS = waitpid.c
waitpid_DEFS = -Dwaitpid_main=main
waitpid_INCS = -I ../libpfkutil

PROG_TARGETS += xtermbar
xtermbar_TARGET = $(OBJDIR)/xtermbar
xtermbar_CSRCS = xtermbar.c
xtermbar_DEFS = -Dxtermbar_main=main
xtermbar_INCS = -I ../libpfkutil

PROG_TARGETS += spinner
spinner_TARGET = $(OBJDIR)/spinner
spinner_CXXSRCS = spinner.cc
spinner_DEFS = -Dspinner_main=main
spinner_INCS = -I ../libpfkutil
spinner_LIBS = -lpthread

include ../Makefile.inc
