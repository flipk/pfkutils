
OBJDIR = obj

PROG_TARGETS += slowlink
slowlink_TARGET = $(OBJDIR)/slowlink
slowlink_CXXSRCS = \
	slowlink.cc \
	../libpfkutil/thread_slinger.cc \
	../libpfkutil/signal_backtrace.cc
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

PROG_TARGETS += find_pfk_pf
find_pfk_pf_TARGET = $(OBJDIR)/find-pfk-pf
find_pfk_pf_CSRCS = find-pfk-pf.c

PROG_TARGETS += random_hex
random_hex_TARGET = $(OBJDIR)/random-hex
random_hex_CSRCS = random-hex.c
random_hex_DEFS = -Drandom_hex_main=main

PROG_TARGETS += random_text
random_text_TARGET = $(OBJDIR)/random-text
random_text_CSRCS = random-hex.c
random_text_DEFS = -Drandom_text_main=main

PROG_TARGETS += rmstar
rmstar_TARGET = $(OBJDIR)/rmstar
rmstar_CXXSRCS = rmstar_new.cc
rmstar_DEFS = -Drmstar_main=main

include ../Makefile.inc
