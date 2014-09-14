
INCS= -I.. -I../../libpfkutil -I../../libpfkdll2

SRCS= proxyServerConn.cc proxyClientTcpAcceptor.cc \
	proxyServerMain.cc proxyClientMain.cc

SERVER_OBJS= proxyServerMain.o proxyServerConn.o proxyMsgs.pb.o
CLIENT_OBJS= proxyClientMain.o proxyClientConn.o proxyMsgs.pb.o \
	proxyClientTcpAcceptor.o
LIBS= -L$(HOME)/pfk/$(PFKARCH)/lib -lWebAppServer -lpfkutil \
	-Wl,-rpath -Wl,$(HOME)/pfk/$(PFKARCH)/lib \
	-lprotobuf -lpthread 

CPPFLAGS= -O3

PROGS= wsProxyServer wsProxyClient

all: xmakefile
	+make -f xmakefile $(PROGS)

xmakefile: proxyMsgs.pb.h Makefile $(SRCS)
	rm -f tmp
	cat Makefile > tmp
	echo '' >> tmp
	g++ $(INCS) -M $(SRCS) >> tmp
	mv tmp xmakefile

proxyMsgs.pb.h proxyMsgs.pb.cc : proxyMsgs.proto
	protoc --cpp_out=. proxyMsgs.proto

wsProxyServer: $(SERVER_OBJS)
	g++ $(SERVER_OBJS) $(LIBS) -o wsProxyServer

wsProxyClient: $(CLIENT_OBJS)
	g++ $(CLIENT_OBJS) $(LIBS) -o wsProxyClient

.cc.o:
	g++ $(CPPFLAGS) -c $(INCS) $*.cc

install:
	cp $(PROGS) $(HOME)/pfk/$(PFKARCH)/bin

clean:
	rm -f proxyMsgs.pb.h proxyMsgs.pb.cc xmakefile
	rm -f $(SERVER_OBJS) $(CLIENT_OBJS) $(PROGS)

