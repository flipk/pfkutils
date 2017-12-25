
ifeq ($(CONFIG),)
CONFIG_FILE := $(wildcard $(HOME)/.pfkutils_config)
ifneq ($(CONFIG_FILE),)
CONFIG := $(shell cat $(CONFIG_FILE))
endif
endif


OBJDIR = obj

ifeq ($(CONFIG),blade)
PROTOC_PATH = /bin/protoc
PROTOBUF_INC =
PROTOBUF_LIB = -lprotobuf
MBEDTLS_INCS =
MBEDTLS_LIBS = -lmbedtls -lmbedx509 -lmbedcrypto
else
ifeq ($(CONFIG),latec)
PROTOC_PATH = /usr/bin/protoc
PROTOBUF_INC =
PROTOBUF_LIB = -lprotobuf
mbedtls_prefix = $(HOME)/pfk/$(PFKARCH)/mbedtls-2.6.1
MBEDTLS_INCS = -I$(mbedtls_prefix)/include
MBEDTLS_LIBS = -L$(mbedtls_prefix)/lib -lmbedtls -lmbedx509 -lmbedcrypto
else
endif
endif

LIB_TARGETS += libprotossl libthreadslinger

libprotossl_TARGET = $(OBJDIR)/libprotossl.a
libprotossl_CXXSRCS = ../libprotossl/libprotossl.cc
libprotossl_INCS = -I../libpfkutil $(MBEDTLS_INCS) $(PROTOBUF_INC)

libthreadslinger_TARGET = $(OBJDIR)/libthreadslinger.a
libthreadslinger_CXXSRCS = ../libpfkutil/thread_slinger.cc
libthreadslinger_INCS = -I../libpfkutil $(MBEDTLS_INCS) $(PROTOBUF_INC)

PROG_TARGETS += i3

i3_TARGET = $(OBJDIR)/i3
i3_PROTOSRCS = i3.proto
i3_CXXSRCS = i3_reader.cc i3_options.cc i3_protossl_factory.cc i3_protossl_conn.cc i3_loop.cc i3_main.cc
i3_INCS = -I../libprotossl -I../libpfkutil $(MBEDTLS_INCS) $(PROTOBUF_INC)
i3_DEFS = -Di3_main=main -DI3_PROTO_HDR=\"i3-i3.pb.h\"
i3_DEPLIBS = $(libi3_TARGET) $(libprotossl_TARGET) $(libthreadslinger_TARGET)
i3_LIBS = $(PROTOBUF_LIB) $(MBEDTLS_LIBS) -lpthread

include ../Makefile.inc
