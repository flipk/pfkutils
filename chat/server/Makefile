
tools= $(HOME)/pfk
webhtml= ../../html-www.phillipknaack.com-secure

proto2js= $(tools)/bin/proto2js
minify= $(tools)/bin/minify
protoc= $(tools)/bin/protoc
libs= ../libWebAppServer/libWebAppServer.a -L$(tools)/lib -lprotobuf -lpthread
incs= -I../libWebAppServer -I$(tools)/include
flags= -Wall -Werror -g3
jsproto_full= $(webhtml)/pfkchat/messages.js
jsproto_login= $(webhtml)/pfkchat_login/messages.js

OBJS= pfkchat-messages.pb.o chatApp.o passwordDatabase.o main.o

DEPS= $(OBJS:.o=.d)

LIBS= ../libWebAppServer/libWebAppServer.a

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

server_pfkchat: $(OBJS) ../libWebAppServer/libWebAppServer.a
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
	cd ../libWebAppServer && make clean
