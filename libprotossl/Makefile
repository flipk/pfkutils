
OBJDIR = obj

LIB_TARGETS = protossl

protossl_TARGET = $(OBJDIR)/libprotossl.a
protossl_CXXSRCS = protosslcertparams.cc protosslconnclient.cc \
	protosslconnserver.cc protossldtlsqueuing.cc protosslmsgs.cc
protossl_PROTOSRCS = protossldtlsqueuing.proto
protossl_INCS = -I../libpfkutil
protossl_DEFS = \
	-DPROTODTLSQUEUING_HDR=\"$(protossl_protossldtlsqueuing.proto_HDR)\"

PROG_TARGETS = protossl_test

protossl_test_TARGET = $(OBJDIR)/protossl_test
protossl_test_CXXSRCS = test.cc ../libpfkutil/signal_backtrace.cc ../libpfkutil/thread_slinger.cc ../libpfkutil/dll3.cc
protossl_test_PROTOSRCS = test_proto.proto
protossl_test_INCS = -I../libpfkutil
protossl_test_DEFS = \
	-DTEST_PROTO_HDR=\"$(protossl_test_test_proto.proto_HDR)\"
protossl_test_DEPLIBS = $(protossl_TARGET)
protossl_test_LIBS = -lpthread -lprotobuf -lmbedtls -lmbedx509 -lmbedcrypto
protossl_test_LDFLAGS = -rdynamic

include ../Makefile.inc
