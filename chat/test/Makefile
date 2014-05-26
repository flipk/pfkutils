
HDRS=         msgs.pb.h
SRCS= test.cc msgs.pb.cc
LIBPB= /home/flipk/proj/protobuf/installed
LIBWAS= ../../libWebAppServer
INCS= -I$(LIBWAS) -I$(LIBPB)/include
LIBFILES=$(LIBWAS)/build/libWebAppServer.a
LIBS= $(LIBFILES) -L$(LIBPB)/lib -lprotobuf -Wl,-R,$(LIBPB)/lib -lpthread

PROTO2JS=/home/flipk/proj/node/installed/bin/proto2js
MSGSJS= ../html/test/msgs.js

PROG= t
OBJS= $(SRCS:.cc=.o)

all: xmakefile
	make -f xmakefile $(PROG) $(MSGSJS)

xmakefile: Makefile $(HDRS) $(SRCS) 
	cat Makefile > tmp
	echo '' >> tmp
	echo '' >> tmp
	g++ -M $(INCS) $(SRCS)  >> tmp
	mv tmp xmakefile

$(MSGSJS) : msgs.proto
	$(PROTO2JS) msgs.proto -class > $(MSGSJS)

.cc.o:
	g++ $(INCS) -c $*.cc

msgs.pb.cc msgs.pb.h : msgs.proto
	$(LIBPB)/bin/protoc --cpp_out=. msgs.proto

$(PROG): $(OBJS) $(LIBFILES)
	g++ $(OBJS) $(LIBS) -o $(PROG)

clean:
	rm -f *.o $(PROG) xmakefile msgs.pb.cc msgs.pb.h
