
OBJDIR = obj

LIB_TARGETS += simpleWebSocket

CXXFLAGS = -fno-diagnostics-color

simpleWebSocket_TARGET = $(OBJDIR)/libsimpleWebSocket.a
simpleWebSocket_CXXSRCS = \
	simpleUrl.cc \
	simpleWebSocket.cc \
	simpleWebSocketClientConn.cc \
	simpleWebSocketServerConn.cc
simpleWebSocket_CSRCS = \
	../libpfkutil/base64.c
PROG_TARGETS += wstest

wstest_TARGET = $(OBJDIR)/simpleWebSocketTest
wstest_CXXSRCS = simpleWebSocketTest.cc
wstest_DEFS = -DPROXYMSGS_PB_H=\"obj/wstest-proxyTcpServer_proxyMsgs.pb.h\"
wstest_PROTOSRCS = proxyTcpServer/proxyMsgs.proto
wstest_DEPLIBS = $(simpleWebSocket_TARGET)
wstest_LIBS = -lprotobuf -lmbedcrypto

include ../Makefile.inc
