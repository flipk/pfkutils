
OBJDIR = obj

LIB_TARGETS += libi3 libprotossl

libi3_TARGET = $(OBJDIR)/libi3.a
libi3_DEFS = -DI3_PROTO_HDR=\"libi3-i3.pb.h\"
libi3_PROTOSRCS = i3.proto
libi3_CXXSRCS = i3_main.cc i3_options.cc i3_protossl_factory.cc
libi3_INCS = -I../libprotossl -I../libpfkutil

libprotossl_TARGET = $(OBJDIR)/libprotossl.a
libprotossl_CXXSRCS = ../libprotossl/libprotossl.cc
libprotossl_INCS = -I../libpfkutil

PROG_TARGETS += i3

i3_TARGET = $(OBJDIR)/i3
i3_CXXSRCS = i3_main.cc
i3_INCS = -I../libprotossl -I../libpfkutil
i3_DEFS = -Di3_main=main -DI3_PROTO_HDR=\"libi3-i3.pb.h\"
i3_DEPLIBS = $(libi3_TARGET) $(libprotossl_TARGET)
i3_LIBS = -lprotobuf -lmbedtls -lmbedx509 -lmbedcrypto -lpthread

include ../Makefile.inc
