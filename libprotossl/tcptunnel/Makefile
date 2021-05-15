
OBJDIR = obj
PFKUTILS_OBJDIR = ../../obj.blade
LIB_TARGETS = tunnelproto extrautils
PROG_TARGETS = main

tunnelproto_TARGET = $(OBJDIR)/libtunnelproto.a
tunnelproto_PROTOSRCS = tcptunnel.proto

extrautils_TARGET = $(OBJDIR)/libextrautils.a
extrautils_CXXSRCS = \
	../../libpfkutil/signal_backtrace.cc \
	../../libpfkutil/thread_slinger.cc \
	../../libpfkutil/dll3.cc

main_TARGET = $(OBJDIR)/tcptunnel
main_CXXSRCS = \
	tuples.cc \
	certs.cc \
	tcptunnel.cc \
	tcptunnel_main.cc \
	tcptunnel_threadstarter.cc \
	tcptunnel_client_thread.cc \
	tcptunnel_proxy.cc \
	main.cc
main_INCS = -I.. -I../../libpfkutil -I$(PFKUTILS_OBJDIR)
main_DEFS = -DTCPTUNNEL_PB_H=\"$(tunnelproto_tcptunnel.proto_HDR)\"
main_DEPLIBS = $(tunnelproto_TARGET) $(extrautils_TARGET)
main_LIBS = $(PFKUTILS_OBJDIR)/libprotossl.a -lpthread -lprotobuf \
	-lmbedtls -lmbedcrypto -lmbedx509

include ../../Makefile.inc
