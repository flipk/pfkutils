# -*- Mode:makefile-gmake; tab-width:8 -*-

ifndef CONFIG
$(error please set CONFIG to a hostname)
endif

CONFIG_VALUES= \
        HAVE_NCURSES_CURSES_H HAVE_NCURSES_H HAVE_CURSES_H 

ifeq ($(CONFIG),blade)

PFK_CONFIG_HAVE_NCURSES_CURSES_H= 1
PFK_CONFIG_HAVE_NCURSES_H= 0
PFK_CONFIG_HAVE_CURSES_H= 0

else

# put other CONFIGs here, although i don't expect to be
# building pfksh standalone in many places other than blade.
$(error unsupported CONFIG value, please edit Makefile)

endif

CONFIG_H= $(OBJDIR)/pfkutils_config.h
BUILD_DATE = $(shell date '+%Y-%m%d-%H%M%S')

CC=gcc
CXX=g++
INCS=
DEFS=
AR=ar
RANLIB=ranlib
PREPROC_TARGETS=
LIB_TARGETS=
PROG_TARGETS= 

PROG_TARGETS += pfksh

CFLAGS = -O0 -g3
CXXFLAGS = -O0 -g3

pfksh_TARGET = $(OBJDIR)/pfksh
pfksh_CSRCS = \
	alloc.c c_ksh.c c_sh.c c_test.c c_ulimit.c edit.c \
	emacs.c eval.c exec.c expr.c history.c io.c jobs.c \
	lex.c main.c misc.c path.c shf.c syn.c table.c trap.c \
	tree.c tty.c var.c version.c
pfksh_HDRS = \
	config.h edit.h emacs_autogen.h expand.h lex.h \
	proto.h sh.h shf.h siglist_autogen.h table.h tree.h tty.h
pfksh_CXXSRCS = xtree.cc
pfksh_DEFS = -Dpfksh_main=main -DBUILD_DATE=\"pfksh-$(BUILD_DATE)\"
pfksh_LIBS = -lcurses

include ../Makefile.inc

define PFK_CONFIG_LINE
	if [ "x$(PFK_CONFIG_$(value))" == "x" ] ; then \
	   echo 'plz define PFK_CONFIG_$(value) in config/$(CONFIG)' ; \
	   exit 1 ; \
	elif [ "$(PFK_CONFIG_$(value))" != "0" ] ; then \
	   echo \#define $(value) $(PFK_CONFIG_$(value)) >> $(CONFIG_H).tmp ; \
	fi ; 
endef

$(CONFIG_H): Makefile
	$(Q)$(foreach value,$(CONFIG_VALUES),$(PFK_CONFIG_LINE))
	$(Q)set -e ; if [ -f $(CONFIG_H) ] ; then \
		if ! cmp -s $(CONFIG_H).tmp $(CONFIG_H) ; then \
			echo making $(CONFIG_H) ; \
			mv $(CONFIG_H).tmp $(CONFIG_H) ; \
		else \
			rm -f $(CONFIG_H).tmp ; \
		fi ; \
	else \
		echo making $(CONFIG_H) ; \
		mv $(CONFIG_H).tmp $(CONFIG_H) ; \
	fi
