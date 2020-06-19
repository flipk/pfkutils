
ifneq ($(wildcard $(HOME)/.pfkutils_config.$(CONFIG)),)
CONFIG_FILE := $(HOME)/.pfkutils_config.$(CONFIG)
else ifneq ($(wildcard $(HOME)/.pfkutils_config),)
CONFIG_FILE := $(HOME)/.pfkutils_config
endif

ifneq ($(CONFIG_FILE),)
include $(CONFIG_FILE)
endif

OBJDIR = obj

LIB_TARGETS = protossl

protossl_TARGET = $(OBJDIR)/libprotossl.a
protossl_CXXSRCS = \
	protosslcertparams.cc \
	protosslconnclient.cc \
	protosslconnserver.cc \
	protossldtlsqueuing.cc \
	protosslmsgs.cc
protossl_PROTOSRCS = protossldtlsqueuing.proto
protossl_INCS = \
	-I../libpfkutil -I../obj.$(CONFIG) \
	$(PROTOBUF_INC) $(MBEDTLS_INCS)
protossl_DEFS = \
	-DPROTODTLSQUEUING_HDR=\"$(protossl_protossldtlsqueuing.proto_HDR)\"

PROG_TARGETS = protossl_test udp_proxy

protossl_test_TARGET = $(OBJDIR)/protossl_test
protossl_test_CXXSRCS = \
	test.cc \
	../libpfkutil/signal_backtrace.cc \
	../libpfkutil/thread_slinger.cc \
	../libpfkutil/dll3.cc
protossl_test_PROTOSRCS = test_proto.proto
protossl_test_INCS = \
	-I../libpfkutil -I../obj.$(CONFIG) \
	$(PROTOBUF_INC) $(MBEDTLS_INCS)
protossl_test_DEFS = \
	-DTEST_PROTO_HDR=\"$(protossl_test_test_proto.proto_HDR)\"
protossl_test_DEPLIBS = $(protossl_TARGET)
protossl_test_LIBS = -lpthread $(PROTOBUF_LIB) $(MBEDTLS_LIBS) -lrt
protossl_test_LDFLAGS = -rdynamic

udp_proxy_TARGET = $(OBJDIR)/udp_proxy
udp_proxy_CXXSRCS = ../misc/udp_proxy.cc
udp_proxy_INCS = -I../libpfkutil
udp_proxy_LIBS = -lpthread -lrt

include ../Makefile.inc
