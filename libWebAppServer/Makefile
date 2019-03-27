
OBJDIR = obj

CXXFLAGS = -fno-diagnostics-color

VERBOSE = -DVERBOSE=0

LIB_TARGETS += simpleWebSocket
simpleWebSocket_TARGET = $(OBJDIR)/libsimpleWebSocket.a
simpleWebSocket_CXXSRCS = \
	simpleUrl.cc \
	simpleWebSocket.cc \
	simpleWebSocketClientConn.cc \
	simpleWebSocketServerConn.cc
simpleWebSocket_CSRCS = \
	../libpfkutil/base64.c
simpleWebSocket_DEFS = $(VERBOSE)
simpleWebSocket_INCS = -I../libpfkutil

PROG_TARGETS += wstest_server
wstest_server_TARGET = $(OBJDIR)/simpleWebSocketTestServer
wstest_server_CXXSRCS = simpleWebSocketTestServer.cc
wstest_server_DEFS = -DPROXYMSGS_PB_H=\"$(OBJDIR)/wstest_server-proxyTcpServer_proxyMsgs.pb.h\" $(VERBOSE)
wstest_server_PROTOSRCS = proxyTcpServer/proxyMsgs.proto
wstest_server_DEPLIBS = $(simpleWebSocket_TARGET)
wstest_server_LIBS = -lprotobuf -lmbedcrypto -lpthread

PROG_TARGETS += wstest_client
wstest_client_TARGET = $(OBJDIR)/simpleWebSocketTestClient
wstest_client_CXXSRCS = simpleWebSocketTestClient.cc
wstest_client_DEFS = -DPROXYMSGS_PB_H=\"$(OBJDIR)/wstest_client-proxyTcpServer_proxyMsgs.pb.h\"  $(VERBOSE)
wstest_client_PROTOSRCS = proxyTcpServer/proxyMsgs.proto
wstest_client_DEPLIBS = $(simpleWebSocket_TARGET)
wstest_client_LIBS = -lprotobuf -lmbedcrypto -lpthread

include ../Makefile.inc
