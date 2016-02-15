# -*- Mode:makefile-gmake; tab-width:8 -*-

OBJDIR= obj.$(PFKARCH)

PFKLIBDIR= $(HOME)/pfk/$(PFKARCH)/lib/

#

LIB_TARGETS += proxyProtos
proxyProtos_TARGET = $(OBJDIR)/libproxyProtos.a
proxyProtos_PROTOSRCS = proxyMsgs.proto

#

PROG_TARGETS += wsProxyServer
wsProxyServer_TARGET = $(OBJDIR)/wsProxyServer
wsProxyServer_CXXSRCS = proxyServerMain.cc proxyServerConn.cc
wsProxyServer_INCS = -I.. -I../../libpfkutil
wsProxyServer_LIBS = \
	-lprotobuf -lpthread \
	-L$(PFKLIBDIR) -lWebAppServer -lpfkutil
wsProxyServer_DEPLIBS = $(proxyProtos_TARGET)

#

PROG_TARGETS += wsProxyClient
wsProxyClient_TARGET = $(OBJDIR)/wsProxyClient
wsProxyClient_CXXSRCS = \
	proxyClientMain.cc proxyClientConn.cc proxyClientTcpAcceptor.cc
wsProxyClient_INCS = -I.. -I../../libpfkutil
wsProxyClient_LIBS = \
	-lprotobuf -lpthread \
	-L$(PFKLIBDIR) -lWebAppServer -lpfkutil
wsProxyClient_DEPLIBS = $(proxyProtos_TARGET)

#

include ../../Makefile.inc
