noinst_LIBRARIES = libWebAppServer.a
noinst_PROGRAMS = wsServerTest wsClientTest

CC = gcc
CXX = g++
libWebAppServer_a_CC = $(CC)
libWebAppServer_a_CXX = $(CXX)
libWebAppServer_a_CFLAGS = -g3 -Wall -Werror
libWebAppServer_a_CXXFLAGS = -g3 -Wall -Werror
libWebAppServer_a_CPPFLAGS = -I$(HOME)/proj/pfkutils/libpfkutil
wsServerTest_CC = $(CC)
wsServerTest_CXX = $(CXX)
wsServerTest_CXXFLAGS = -g3 -Wall -Werror
wsServerTest_LDADD = libWebAppServer.a
wsServerTest_LDFLAGS = -lpthread
wsServerTest_CPPFLAGS = -I$(HOME)/proj/pfkutils/libpfkutil
wsClientTest_CC = $(CC)
wsClientTest_CXX = $(CXX)
wsClientTest_CXXFLAGS = -g3 -Wall -Werror
wsClientTest_LDADD = libWebAppServer.a
wsClientTest_LDFLAGS = -lpthread
wsClientTest_CPPFLAGS = -I$(HOME)/proj/pfkutils/libpfkutil
srcdir = /home/flipk/proj/pfkutils/libWebAppServer
builddir = /home/flipk/proj/pfkutils/libWebAppServer
CPPFLAGS = -I$(srcdir)

all: xmakefile
	+make $(MAKEJOBS) -f xmakefile libWebAppServer.a wsServerTest wsClientTest

xmakefile: Makefile libWebAppServer_a-base64.d libWebAppServer_a-md5.d libWebAppServer_a-sha1.d libWebAppServer_a-fdThreadLauncher.d libWebAppServer_a-serverPorts.d libWebAppServer_a-WebAppConnection.d libWebAppServer_a-WebAppServer.d libWebAppServer_a-WebAppServerConfig.d libWebAppServer_a-WebServerConnectionBase.d libWebAppServer_a-WebFastCGIConnection.d libWebAppServer_a-WebSocketConnection.d libWebAppServer_a-WebSocketClient.d wsServerTest-wsServerTest.d wsClientTest-wsClientTest.d
	rm -f xmakefile xmakefile-tmp
	cat Makefile > xmakefile-tmp
	echo '' >> xmakefile-tmp
	cat libWebAppServer_a-base64.d libWebAppServer_a-md5.d libWebAppServer_a-sha1.d libWebAppServer_a-fdThreadLauncher.d libWebAppServer_a-serverPorts.d libWebAppServer_a-WebAppConnection.d libWebAppServer_a-WebAppServer.d libWebAppServer_a-WebAppServerConfig.d libWebAppServer_a-WebServerConnectionBase.d libWebAppServer_a-WebFastCGIConnection.d libWebAppServer_a-WebSocketConnection.d libWebAppServer_a-WebSocketClient.d wsServerTest-wsServerTest.d wsClientTest-wsClientTest.d >> xmakefile-tmp
	mv xmakefile-tmp xmakefile

Makefile: /home/flipk/proj/pfkutils/libWebAppServer/Makefile.am
	ampfk /home/flipk/proj/pfkutils/libWebAppServer/Makefile.am
	make clean

libWebAppServer_a-base64.d: $(srcdir)/base64.cc
	$(libWebAppServer_a_CXX) $(CXXFLAGS) $(libWebAppServer_a_CXXFLAGS) $(CPPFLAGS) $(libWebAppServer_a_CPPFLAGS) -MT ' libWebAppServer_a-base64.o libWebAppServer_a-base64.d ' -M -MF libWebAppServer_a-base64.d $(srcdir)/base64.cc

libWebAppServer_a-md5.d: $(srcdir)/md5.cc
	$(libWebAppServer_a_CXX) $(CXXFLAGS) $(libWebAppServer_a_CXXFLAGS) $(CPPFLAGS) $(libWebAppServer_a_CPPFLAGS) -MT ' libWebAppServer_a-md5.o libWebAppServer_a-md5.d ' -M -MF libWebAppServer_a-md5.d $(srcdir)/md5.cc

libWebAppServer_a-sha1.d: $(srcdir)/sha1.cc
	$(libWebAppServer_a_CXX) $(CXXFLAGS) $(libWebAppServer_a_CXXFLAGS) $(CPPFLAGS) $(libWebAppServer_a_CPPFLAGS) -MT ' libWebAppServer_a-sha1.o libWebAppServer_a-sha1.d ' -M -MF libWebAppServer_a-sha1.d $(srcdir)/sha1.cc

libWebAppServer_a-fdThreadLauncher.d: $(srcdir)/fdThreadLauncher.cc
	$(libWebAppServer_a_CXX) $(CXXFLAGS) $(libWebAppServer_a_CXXFLAGS) $(CPPFLAGS) $(libWebAppServer_a_CPPFLAGS) -MT ' libWebAppServer_a-fdThreadLauncher.o libWebAppServer_a-fdThreadLauncher.d ' -M -MF libWebAppServer_a-fdThreadLauncher.d $(srcdir)/fdThreadLauncher.cc

libWebAppServer_a-serverPorts.d: $(srcdir)/serverPorts.cc
	$(libWebAppServer_a_CXX) $(CXXFLAGS) $(libWebAppServer_a_CXXFLAGS) $(CPPFLAGS) $(libWebAppServer_a_CPPFLAGS) -MT ' libWebAppServer_a-serverPorts.o libWebAppServer_a-serverPorts.d ' -M -MF libWebAppServer_a-serverPorts.d $(srcdir)/serverPorts.cc

libWebAppServer_a-WebAppConnection.d: $(srcdir)/WebAppConnection.cc
	$(libWebAppServer_a_CXX) $(CXXFLAGS) $(libWebAppServer_a_CXXFLAGS) $(CPPFLAGS) $(libWebAppServer_a_CPPFLAGS) -MT ' libWebAppServer_a-WebAppConnection.o libWebAppServer_a-WebAppConnection.d ' -M -MF libWebAppServer_a-WebAppConnection.d $(srcdir)/WebAppConnection.cc

libWebAppServer_a-WebAppServer.d: $(srcdir)/WebAppServer.cc
	$(libWebAppServer_a_CXX) $(CXXFLAGS) $(libWebAppServer_a_CXXFLAGS) $(CPPFLAGS) $(libWebAppServer_a_CPPFLAGS) -MT ' libWebAppServer_a-WebAppServer.o libWebAppServer_a-WebAppServer.d ' -M -MF libWebAppServer_a-WebAppServer.d $(srcdir)/WebAppServer.cc

libWebAppServer_a-WebAppServerConfig.d: $(srcdir)/WebAppServerConfig.cc
	$(libWebAppServer_a_CXX) $(CXXFLAGS) $(libWebAppServer_a_CXXFLAGS) $(CPPFLAGS) $(libWebAppServer_a_CPPFLAGS) -MT ' libWebAppServer_a-WebAppServerConfig.o libWebAppServer_a-WebAppServerConfig.d ' -M -MF libWebAppServer_a-WebAppServerConfig.d $(srcdir)/WebAppServerConfig.cc

libWebAppServer_a-WebServerConnectionBase.d: $(srcdir)/WebServerConnectionBase.cc
	$(libWebAppServer_a_CXX) $(CXXFLAGS) $(libWebAppServer_a_CXXFLAGS) $(CPPFLAGS) $(libWebAppServer_a_CPPFLAGS) -MT ' libWebAppServer_a-WebServerConnectionBase.o libWebAppServer_a-WebServerConnectionBase.d ' -M -MF libWebAppServer_a-WebServerConnectionBase.d $(srcdir)/WebServerConnectionBase.cc

libWebAppServer_a-WebFastCGIConnection.d: $(srcdir)/WebFastCGIConnection.cc
	$(libWebAppServer_a_CXX) $(CXXFLAGS) $(libWebAppServer_a_CXXFLAGS) $(CPPFLAGS) $(libWebAppServer_a_CPPFLAGS) -MT ' libWebAppServer_a-WebFastCGIConnection.o libWebAppServer_a-WebFastCGIConnection.d ' -M -MF libWebAppServer_a-WebFastCGIConnection.d $(srcdir)/WebFastCGIConnection.cc

libWebAppServer_a-WebSocketConnection.d: $(srcdir)/WebSocketConnection.cc
	$(libWebAppServer_a_CXX) $(CXXFLAGS) $(libWebAppServer_a_CXXFLAGS) $(CPPFLAGS) $(libWebAppServer_a_CPPFLAGS) -MT ' libWebAppServer_a-WebSocketConnection.o libWebAppServer_a-WebSocketConnection.d ' -M -MF libWebAppServer_a-WebSocketConnection.d $(srcdir)/WebSocketConnection.cc

libWebAppServer_a-WebSocketClient.d: $(srcdir)/WebSocketClient.cc
	$(libWebAppServer_a_CXX) $(CXXFLAGS) $(libWebAppServer_a_CXXFLAGS) $(CPPFLAGS) $(libWebAppServer_a_CPPFLAGS) -MT ' libWebAppServer_a-WebSocketClient.o libWebAppServer_a-WebSocketClient.d ' -M -MF libWebAppServer_a-WebSocketClient.d $(srcdir)/WebSocketClient.cc

wsServerTest-wsServerTest.d: $(srcdir)/wsServerTest.cc
	$(wsServerTest_CXX) $(CXXFLAGS) $(wsServerTest_CXXFLAGS) $(CPPFLAGS) $(wsServerTest_CPPFLAGS) -MT ' wsServerTest-wsServerTest.o wsServerTest-wsServerTest.d ' -M -MF wsServerTest-wsServerTest.d $(srcdir)/wsServerTest.cc

wsClientTest-wsClientTest.d: $(srcdir)/wsClientTest.cc
	$(wsClientTest_CXX) $(CXXFLAGS) $(wsClientTest_CXXFLAGS) $(CPPFLAGS) $(wsClientTest_CPPFLAGS) -MT ' wsClientTest-wsClientTest.o wsClientTest-wsClientTest.d ' -M -MF wsClientTest-wsClientTest.d $(srcdir)/wsClientTest.cc

libWebAppServer.a: libWebAppServer_a-base64.o libWebAppServer_a-md5.o libWebAppServer_a-sha1.o libWebAppServer_a-fdThreadLauncher.o libWebAppServer_a-serverPorts.o libWebAppServer_a-WebAppConnection.o libWebAppServer_a-WebAppServer.o libWebAppServer_a-WebAppServerConfig.o libWebAppServer_a-WebServerConnectionBase.o libWebAppServer_a-WebFastCGIConnection.o libWebAppServer_a-WebSocketConnection.o libWebAppServer_a-WebSocketClient.o $(libWebAppServer_a_LDADD)
	rm -f libWebAppServer.a
	$(AR) cq libWebAppServer.a  libWebAppServer_a-base64.o libWebAppServer_a-md5.o libWebAppServer_a-sha1.o libWebAppServer_a-fdThreadLauncher.o libWebAppServer_a-serverPorts.o libWebAppServer_a-WebAppConnection.o libWebAppServer_a-WebAppServer.o libWebAppServer_a-WebAppServerConfig.o libWebAppServer_a-WebServerConnectionBase.o libWebAppServer_a-WebFastCGIConnection.o libWebAppServer_a-WebSocketConnection.o libWebAppServer_a-WebSocketClient.o $(libWebAppServer_a_LDADD) $(libWebAppServer_a_LDFLAGS)

wsServerTest: wsServerTest-wsServerTest.o $(wsServerTest_LDADD)
	rm -f wsServerTest
	$(wsServerTest_CXX) $(CXXFLAGS) $(wsServerTest_CXXFLAGS) -o wsServerTest  wsServerTest-wsServerTest.o $(wsServerTest_LDADD) $(wsServerTest_LDFLAGS)

wsClientTest: wsClientTest-wsClientTest.o $(wsClientTest_LDADD)
	rm -f wsClientTest
	$(wsClientTest_CXX) $(CXXFLAGS) $(wsClientTest_CXXFLAGS) -o wsClientTest  wsClientTest-wsClientTest.o $(wsClientTest_LDADD) $(wsClientTest_LDFLAGS)

libWebAppServer_a-base64.o: $(srcdir)/base64.cc
	$(libWebAppServer_a_CXX) $(CXXFLAGS) $(libWebAppServer_a_CXXFLAGS) $(CPPFLAGS) $(libWebAppServer_a_CPPFLAGS) -c $(srcdir)/base64.cc -o libWebAppServer_a-base64.o

libWebAppServer_a-md5.o: $(srcdir)/md5.cc
	$(libWebAppServer_a_CXX) $(CXXFLAGS) $(libWebAppServer_a_CXXFLAGS) $(CPPFLAGS) $(libWebAppServer_a_CPPFLAGS) -c $(srcdir)/md5.cc -o libWebAppServer_a-md5.o

libWebAppServer_a-sha1.o: $(srcdir)/sha1.cc
	$(libWebAppServer_a_CXX) $(CXXFLAGS) $(libWebAppServer_a_CXXFLAGS) $(CPPFLAGS) $(libWebAppServer_a_CPPFLAGS) -c $(srcdir)/sha1.cc -o libWebAppServer_a-sha1.o

libWebAppServer_a-fdThreadLauncher.o: $(srcdir)/fdThreadLauncher.cc
	$(libWebAppServer_a_CXX) $(CXXFLAGS) $(libWebAppServer_a_CXXFLAGS) $(CPPFLAGS) $(libWebAppServer_a_CPPFLAGS) -c $(srcdir)/fdThreadLauncher.cc -o libWebAppServer_a-fdThreadLauncher.o

libWebAppServer_a-serverPorts.o: $(srcdir)/serverPorts.cc
	$(libWebAppServer_a_CXX) $(CXXFLAGS) $(libWebAppServer_a_CXXFLAGS) $(CPPFLAGS) $(libWebAppServer_a_CPPFLAGS) -c $(srcdir)/serverPorts.cc -o libWebAppServer_a-serverPorts.o

libWebAppServer_a-WebAppConnection.o: $(srcdir)/WebAppConnection.cc
	$(libWebAppServer_a_CXX) $(CXXFLAGS) $(libWebAppServer_a_CXXFLAGS) $(CPPFLAGS) $(libWebAppServer_a_CPPFLAGS) -c $(srcdir)/WebAppConnection.cc -o libWebAppServer_a-WebAppConnection.o

libWebAppServer_a-WebAppServer.o: $(srcdir)/WebAppServer.cc
	$(libWebAppServer_a_CXX) $(CXXFLAGS) $(libWebAppServer_a_CXXFLAGS) $(CPPFLAGS) $(libWebAppServer_a_CPPFLAGS) -c $(srcdir)/WebAppServer.cc -o libWebAppServer_a-WebAppServer.o

libWebAppServer_a-WebAppServerConfig.o: $(srcdir)/WebAppServerConfig.cc
	$(libWebAppServer_a_CXX) $(CXXFLAGS) $(libWebAppServer_a_CXXFLAGS) $(CPPFLAGS) $(libWebAppServer_a_CPPFLAGS) -c $(srcdir)/WebAppServerConfig.cc -o libWebAppServer_a-WebAppServerConfig.o

libWebAppServer_a-WebServerConnectionBase.o: $(srcdir)/WebServerConnectionBase.cc
	$(libWebAppServer_a_CXX) $(CXXFLAGS) $(libWebAppServer_a_CXXFLAGS) $(CPPFLAGS) $(libWebAppServer_a_CPPFLAGS) -c $(srcdir)/WebServerConnectionBase.cc -o libWebAppServer_a-WebServerConnectionBase.o

libWebAppServer_a-WebFastCGIConnection.o: $(srcdir)/WebFastCGIConnection.cc
	$(libWebAppServer_a_CXX) $(CXXFLAGS) $(libWebAppServer_a_CXXFLAGS) $(CPPFLAGS) $(libWebAppServer_a_CPPFLAGS) -c $(srcdir)/WebFastCGIConnection.cc -o libWebAppServer_a-WebFastCGIConnection.o

libWebAppServer_a-WebSocketConnection.o: $(srcdir)/WebSocketConnection.cc
	$(libWebAppServer_a_CXX) $(CXXFLAGS) $(libWebAppServer_a_CXXFLAGS) $(CPPFLAGS) $(libWebAppServer_a_CPPFLAGS) -c $(srcdir)/WebSocketConnection.cc -o libWebAppServer_a-WebSocketConnection.o

libWebAppServer_a-WebSocketClient.o: $(srcdir)/WebSocketClient.cc
	$(libWebAppServer_a_CXX) $(CXXFLAGS) $(libWebAppServer_a_CXXFLAGS) $(CPPFLAGS) $(libWebAppServer_a_CPPFLAGS) -c $(srcdir)/WebSocketClient.cc -o libWebAppServer_a-WebSocketClient.o

wsServerTest-wsServerTest.o: $(srcdir)/wsServerTest.cc
	$(wsServerTest_CXX) $(CXXFLAGS) $(wsServerTest_CXXFLAGS) $(CPPFLAGS) $(wsServerTest_CPPFLAGS) -c $(srcdir)/wsServerTest.cc -o wsServerTest-wsServerTest.o

wsClientTest-wsClientTest.o: $(srcdir)/wsClientTest.cc
	$(wsClientTest_CXX) $(CXXFLAGS) $(wsClientTest_CXXFLAGS) $(CPPFLAGS) $(wsClientTest_CPPFLAGS) -c $(srcdir)/wsClientTest.cc -o wsClientTest-wsClientTest.o

clean: 
	rm -f xmakefile *.d *.o libWebAppServer.a wsServerTest wsClientTest

