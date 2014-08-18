
INCS= -I.. -I../../libpfkutil/

SRCS= proxyServerConn.cc proxyServerMain.cc proxyClientMain.cc

SERVER_OBJS= proxyServerMain.o proxyServerConn.o proxyMsgs.pb.o
CLIENT_OBJS= proxyClientMain.o proxyClientConn.o proxyMsgs.pb.o
LIBS= ../BUILD/libWebAppServer.a -lprotobuf -lpthread

CPPFLAGS= -g3

PROGS= proxyServer proxyClient

all: xmakefile
	make -f xmakefile $(PROGS)

xmakefile: proxyMsgs.pb.h Makefile $(SRCS)
	rm -f tmp
	cat Makefile > tmp
	echo '' >> tmp
	g++ $(INCS) -M $(SRCS) >> tmp
	mv tmp xmakefile

proxyMsgs.pb.h proxyMsgs.pb.cc : proxyMsgs.proto
	protoc --cpp_out=. proxyMsgs.proto

proxyServer: $(SERVER_OBJS)
	cd ../BUILD && make
	g++ $(SERVER_OBJS) $(LIBS) -o proxyServer

proxyClient: $(CLIENT_OBJS)
	cd ../BUILD && make
	g++ $(CLIENT_OBJS) $(LIBS) -o proxyClient

.cc.o:
	g++ $(CPPFLAGS) -c $(INCS) $*.cc

clean:
	rm -f proxyMsgs.pb.h proxyMsgs.pb.cc
	rm -f $(SERVER_OBJS) $(CLIENT_OBJS) $(PROGS)

