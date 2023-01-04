
OBJDIR = obj

# enable this to see what PROTOSRCS does
# DEBUG_H_MACRO_GENERATION=y

# enable compiling with debug:
# CXXFLAGS = -O0 -g3
# LDFLAGS = -g3

PROG_TARGETS += uuz

uuz_TARGET = $(OBJDIR)/uuz
uuz_CXXSRCS = \
	../libpfkutil/newbase64.cc \
	uuz_options.cc \
	uuz_decode.cc \
	uuz_encode.cc \
	uuz.cc
uuz_PROTOSRCS = uuz.proto
uuz_DEFS = -Duuz_main=main  -DUUZ_PROTO_HDR=\"$(uuz_uuz.proto_HDR)\"
uuz_LIBS = -lprotobuf -lz -lmbedcrypto
uuz_INCS = -I ../libpfkutil

new_all:
	@+make all
	@make alltest

EXTRA_CLEAN += alltest_clean

include ../Makefile.inc

alltest:
	bash test-encode.sh

alltest_clean:
	rm -rf test
