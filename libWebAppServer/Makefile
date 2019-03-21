
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

PROG_TARGETS += wstest
wstest_TARGET = $(OBJDIR)/simpleWebSocketTest
wstest_CXXSRCS = simpleWebSocketTest.cc
wstest_DEFS = -DPROXYMSGS_PB_H=\"obj/wstest-proxyTcpServer_proxyMsgs.pb.h\"
wstest_PROTOSRCS = proxyTcpServer/proxyMsgs.proto
wstest_DEPLIBS = $(simpleWebSocket_TARGET)
wstest_LIBS = -lprotobuf -lmbedcrypto
wstest_INCS = -I../libpfkutil

include ../Makefile.inc
