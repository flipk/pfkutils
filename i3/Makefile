# -*- Mode:makefile-gmake; tab-width:8 -*-

DEBUG_H_MACRO_GENERATION=y

NODENAME := $(shell uname -n)
ifneq ($(wildcard $(HOME)/pfk/etc/pfkutils.config.$(NODENAME)),)
CONFIG_FILE := $(HOME)/pfk/etc/pfkutils.config.$(NODENAME)
else ifneq ($(wildcard $(HOME)/pfk/etc/pfkutils.config.$(CONFIG)),)
CONFIG_FILE := $(HOME)/pfk/etc/pfkutils.config.$(CONFIG)
else ifneq ($(wildcard $(HOME)/pfk/etc/pfkutils.config),)
CONFIG_FILE := $(HOME)/pfk/etc/pfkutils.config
endif

include $(CONFIG_FILE)

OBJDIR = obj

INCS = -I../obj.$(CONFIG)

LIB_TARGETS += libprotossl libutil

libprotossl_TARGET = $(OBJDIR)/libprotossl.a
libprotossl_CXXSRCS = \
	../libprotossl/protosslcertparams.cc \
	../libprotossl/protosslconnclient.cc \
	../libprotossl/protosslconnserver.cc \
	../libprotossl/protossldtlsqueuing.cc \
	../libprotossl/protosslmsgs.cc
libprotossl_PROTOSRCS = ../libprotossl/protossldtlsqueuing.proto
libprotossl_INCS = -I../libpfkutil $(MBEDTLS_INCS) $(PROTOBUF_INC)
libprotossl_DEFS = -DPROTODTLSQUEUING_HDR=\"$(libprotossl_../libprotossl/protossldtlsqueuing.proto_HDR)\"

libutil_TARGET = $(OBJDIR)/libutil.a
libutil_CXXSRCS = \
	../libpfkutil/thread_slinger.cc \
	../libpfkutil/signal_backtrace.cc \
	../libpfkutil/dll3.cc
libutil_INCS = -I../libpfkutil $(MBEDTLS_INCS) $(PROTOBUF_INC)

LIB_TARGETS += i3_lib

i3_lib_TARGET = $(OBJDIR)/i3lib.a
i3_lib_PROTOSRCS = i3.proto
i3_lib_CXXSRCS = i3_options.cc

PROG_TARGETS += i3_udp i3_tcp

i3_udp_TARGET = $(OBJDIR)/i3_udp
i3_udp_CXXSRCS =  i3_main_udp.cc
i3_udp_INCS = -I../libprotossl -I../libpfkutil $(MBEDTLS_INCS) $(PROTOBUF_INC)
i3_udp_DEFS = -Di3_udp_main=main -DI3_PROTO_HDR=\"$(i3_lib_i3.proto_HDR)\"
i3_udp_DEPLIBS = $(i3_lib_TARGET) $(libprotossl_TARGET) $(libutil_TARGET)
i3_udp_LIBS = $(PROTOBUF_LIB) $(MBEDTLS_LIBS) -lpthread
i3_udp_LDFLAGS = -rdynamic

i3_tcp_TARGET = $(OBJDIR)/i3_tcp
i3_tcp_CXXSRCS =  i3_main_tcp.cc
i3_tcp_INCS = -I../libprotossl -I../libpfkutil $(MBEDTLS_INCS) $(PROTOBUF_INC)
i3_tcp_DEFS = -Di3_tcp_main=main -DI3_PROTO_HDR=\"$(i3_lib_i3.proto_HDR)\"
i3_tcp_DEPLIBS = $(i3_lib_TARGET) $(libprotossl_TARGET) $(libutil_TARGET)
i3_tcp_LIBS = $(PROTOBUF_LIB) $(MBEDTLS_LIBS) -lpthread
i3_tcp_LDFLAGS = -rdynamic

include ../Makefile.inc
