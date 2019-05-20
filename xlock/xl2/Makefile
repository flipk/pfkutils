
OBJDIR = obj

LIB_TARGETS += # libxl2

libxl2_TARGET = $(OBJDIR)/libxl2.a
libxl2_CXXSRCS = libxl2.cc


PROG_TARGETS += xl2 get_idle

xl2_TARGET = $(OBJDIR)/xl2
xl2_INCS = -I$(HOME)/proj/pfkutils/libpfkutil
xl2_LIBS = -lX11 -lm
xl2_LDFLAGS = -rdynamic  # needed for backtrace
xl2_CXXSRCS = \
	lock_procs.cc \
	lock_proc_swarm.cc \
	lock_proc_rotor.cc \
	lock_proc_pyro.cc \
	lock_proc_flame.cc \
	lock_proc_worm.cc \
	../../libpfkutil/signal_backtrace.cc \
	read_x_string.cc \
	main.cc

get_idle_TARGET = $(OBJDIR)/get_idle
get_idle_LIBS = -lX11 -lXss -lXext
get_idle_CXXSRCS = idle.cc
get_idle_DEFS = -D__TEST_XSS_IDLE__

include ../../Makefile.inc
