
PFKCONFIG_PATH := $(shell find .. -name pfkutils_config.h | sed -e s,/pfkutils_config.h,,)

ifeq ($(PFKCONFIG_PATH),)
$(error cannot find pfkutils_config.h, did you build at top level?)
else
$(info NOTE using $(PFKCONFIG_PATH) for pfkutils_config.h)
endif

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

PROG_TARGETS += he
he_TARGET = $(OBJDIR)/he
he_CSRCS = he.c ../libpfkutil/m.c
he_DEFS = -Dhe_main=main
he_INCS = -I$(PFKCONFIG_PATH) -I../libpfkutil
he_LIBS = -lcurses

PROG_TARGETS += m
m_TARGET = $(OBJDIR)/m
m_CSRCS = ../libpfkutil/m.c
m_DEFS = -Dm_main=main
m_INCS = -I../libpfkutil

PROG_TARGETS += tickler
tickler_TARGET = $(OBJDIR)/tickler
tickler_CXXSRCS = tickler.cc
tickler_DEFS = -Dtickler_main=main
tickler_INCS = -I../libpfkutil
tickler_LIBS = -lpthread

include ../Makefile.inc
