
LIB_TARGETS += protossl

PROG_TARGETS += t

protossl_TARGET = $(OBJDIR)/libprotossl.a
protossl_CXXSRCS = libprotossl.cc
protossl_INCS = -I$(HOME)/proj/pfkutils/libpfkutil

t_TARGET = $(OBJDIR)/t
t_PROTOSRCS = test_proto.proto
t_CXXSRCS = test.cc
t_INCS = -I$(HOME)/proj/pfkutils/libpfkutil
t_LIBS = -lprotobuf -lpthread -lmbedtls -lmbedx509 -lmbedcrypto
t_DEPLIBS = $(protossl_TARGET)

include $(HOME)/pfk/Makefile.inc
