
OBJDIR = obj

LIB_TARGETS += libprotossl libthreadslinger

libprotossl_TARGET = $(OBJDIR)/libprotossl.a
libprotossl_CXXSRCS = ../libprotossl/libprotossl.cc
libprotossl_INCS = -I../libpfkutil

libthreadslinger_TARGET = $(OBJDIR)/libthreadslinger.a
libthreadslinger_CXXSRCS = ../libpfkutil/thread_slinger.cc
libthreadslinger_INCS = -I../libpfkutil

PROG_TARGETS += i3

i3_TARGET = $(OBJDIR)/i3
i3_PROTOSRCS = i3.proto
i3_CXXSRCS = i3_reader.cc i3_options.cc i3_protossl_factory.cc i3_protossl_conn.cc i3_loop.cc i3_main.cc
i3_INCS = -I../libprotossl -I../libpfkutil
i3_DEFS = -Di3_main=main -DI3_PROTO_HDR=\"i3-i3.pb.h\"
i3_DEPLIBS = $(libi3_TARGET) $(libprotossl_TARGET) $(libthreadslinger_TARGET)
i3_LIBS = -lprotobuf -lmbedtls -lmbedx509 -lmbedcrypto -lpthread

include ../Makefile.inc
