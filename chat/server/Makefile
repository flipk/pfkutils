
proto2js= /home/flipk/proj/node/installed/bin/node /home/flipk/proj/node/installed/bin/proto2js
protobuf= /home/flipk/proj/protobuf/installed
protoc= $(protobuf)/bin/protoc

libwasdir= ../../libWebAppServer
libwas= $(libwasdir)/build/libWebAppServer.a

libs= $(libwas) -L$(protobuf)/lib -lprotobuf -lpthread -Wl,-R,$(protobuf)/lib
incs= -I$(libwasdir) -I$(protobuf)/include
flags= -Wall -Werror -g3
jsproto_full=  pfkchat-messages-full.js
jsproto_login= pfkchat-messages-login.js

OBJS= pfkchat-messages.pb.o chatApp.o passwordDatabase.o main.o

DEPS= $(OBJS:.o=.d)

all: xmakefile
	make -f xmakefile server_pfkchat

xmakefile: Makefile pfkchat-protoversion.h $(DEPS) 
	rm -f xmakefile xmakefile-tmp
	cat Makefile > xmakefile-tmp
	echo '' >> xmakefile-tmp
	cat $(DEPS) >> xmakefile-tmp
	mv xmakefile-tmp xmakefile

.SUFFIXES: .c .cc .d

.c.d:
	gcc $(incs) -M $*.c > $*.d

.cc.d:
	g++ $(incs) -M $*.cc > $*.d

server_pfkchat: $(OBJS) $(libwas)
	g++ $(OBJS) $(libs) -o server_pfkchat
#	rm -f vers

pfkchat-protoversion.h: vers
	echo "#define PFK_CHAT_CurrentProtoVersion `cat vers`" > pfkchat-protoversion.h

vers:
	echo -n 0x > vers
	random_hex 7 >> vers

pfkchat-messages.pb.cc pfkchat-messages.pb.h: pfkchat-messages-all.proto vers
	sed -e s,//PRIVATE,, < pfkchat-messages-all.proto > pfkchat-messages.proto
	$(protoc) --cpp_out=. pfkchat-messages.proto
	rm -f tempfile
	$(proto2js) pfkchat-messages.proto -class > tempfile
	echo "; PFK.Chat.CurrentProtoVersion = `cat vers`;" >> tempfile
	mv tempfile $(jsproto_full)
	grep -v //PRIVATE pfkchat-messages-all.proto > pfkchat-messages-login.proto
	$(proto2js) pfkchat-messages-login.proto -class > tempfile
	echo "; PFK.Chat.CurrentProtoVersion = `cat vers`;" >> tempfile
	mv tempfile $(jsproto_login)

.cc.o:
	g++ $(incs) $(flags) -c $*.cc

.c.o:
	gcc $(incs) $(flags) -c $*.c

clean:
	rm -f *.o *.d *.pb.cc *.pb.h server_pfkchat vers *~ xmakefile
	rm -f pfkchat-protoversion.h
	rm -f pfkchat-messages.proto pfkchat-messages-login.proto
	rm -f $(jsproto_full) $(jsproto_login)
