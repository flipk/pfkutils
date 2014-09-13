
INCS= -I.. -I../../libpfkutil -I../../libpfkdll2

SRCS= proxyServerConn.cc proxyClientTcpAcceptor.cc \
	proxyServerMain.cc proxyClientMain.cc \
	../../libpfkutil/fd_mgr.C

SERVER_OBJS= proxyServerMain.o proxyServerConn.o proxyMsgs.pb.o
CLIENT_OBJS= proxyClientMain.o proxyClientConn.o proxyMsgs.pb.o \
	proxyClientTcpAcceptor.o fd_mgr.o
LIBDEP= ../BUILD/libWebAppServer.a
LIBS= ../BUILD/libWebAppServer.a -lprotobuf -lpthread

CPPFLAGS= -g3

PROGS= wsProxyServer wsProxyClient

all: xmakefile
	+cd ../BUILD && make
	+make -f xmakefile $(PROGS)

xmakefile: proxyMsgs.pb.h Makefile $(SRCS)
	rm -f tmp
	cat Makefile > tmp
	echo '' >> tmp
	g++ $(INCS) -M $(SRCS) >> tmp
	mv tmp xmakefile

proxyMsgs.pb.h proxyMsgs.pb.cc : proxyMsgs.proto
	protoc --cpp_out=. proxyMsgs.proto

wsProxyServer: $(SERVER_OBJS) $(LIBDEP)
	g++ $(SERVER_OBJS) $(LIBS) -o wsProxyServer

wsProxyClient: $(CLIENT_OBJS) $(LIBDEP)
	g++ $(CLIENT_OBJS) $(LIBS) -o wsProxyClient

.cc.o:
	g++ $(CPPFLAGS) -c $(INCS) $*.cc

fd_mgr.o: ../../libpfkutil/fd_mgr.C
	g++ $(CPPFLAGS) -c $(INCS) ../../libpfkutil/fd_mgr.C

clean:
	rm -f proxyMsgs.pb.h proxyMsgs.pb.cc
	rm -f $(SERVER_OBJS) $(CLIENT_OBJS) $(PROGS)

