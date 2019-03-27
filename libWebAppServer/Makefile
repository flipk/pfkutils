
OBJDIR = obj

CXXFLAGS = -fno-diagnostics-color

LIB_TARGETS += simpleWebSocket
simpleWebSocket_TARGET = $(OBJDIR)/libsimpleWebSocket.a
simpleWebSocket_CXXSRCS = \
	simpleUrl.cc \
	simpleWebSocket.cc \
	simpleWebSocketClientConn.cc \
	simpleWebSocketServerConn.cc
simpleWebSocket_CSRCS = \
	../libpfkutil/base64.c
simpleWebSocket_INCS = -I../libpfkutil

PROG_TARGETS += wstest_server
wstest_server_TARGET = $(OBJDIR)/simple_server
wstest_server_CXXSRCS = simpleWsTestServer.cc
wstest_server_PROTOSRCS = proxyTcpServer/proxyMsgs.proto
wstest_server_DEFS = -DPROXYMSGS_PB_H=\"$(wstest_server_proxyTcpServer/proxyMsgs.proto_HDR)\"
wstest_server_DEPLIBS = $(simpleWebSocket_TARGET)
wstest_server_LIBS = -lprotobuf -lmbedcrypto -lpthread

PROG_TARGETS += wstest_client
wstest_client_TARGET = $(OBJDIR)/simple_client
wstest_client_CXXSRCS = simpleWsTestClient.cc
wstest_client_DEFS = -DPROXYMSGS_PB_H=\"$(wstest_client_proxyTcpServer/proxyMsgs.proto_HDR)\"
wstest_client_PROTOSRCS = proxyTcpServer/proxyMsgs.proto
wstest_client_DEPLIBS = $(simpleWebSocket_TARGET)
wstest_client_LIBS = -lprotobuf -lmbedcrypto -lpthread

include ../Makefile.inc
