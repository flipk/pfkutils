noinst_LIBRARIES = libWebAppServer.a

libWebAppServer_a_CC = $(CC)
libWebAppServer_a_CXX = $(CXX)
libWebAppServer_a_CFLAGS = -g3
libWebAppServer_a_CXXFLAGS = -g3
srcdir = /home/flipk/proj/pfkutils/libWebAppServer
builddir = /home/flipk/proj/pfkutils/libWebAppServer
CPPFLAGS = -I$(srcdir)

all: xmakefile
	+make $(MAKEJOBS) -f xmakefile libWebAppServer.a

xmakefile: Makefile libWebAppServer_a-base64.d libWebAppServer_a-md5.d libWebAppServer_a-sha1.d libWebAppServer_a-fdThreadLauncher.d libWebAppServer_a-serverPorts.d libWebAppServer_a-WebAppConnection.d libWebAppServer_a-WebAppServer.d libWebAppServer_a-WebAppServerConfig.d libWebAppServer_a-WebServerConnectionBase.d libWebAppServer_a-WebFastCGIConnection.d libWebAppServer_a-WebSocketConnection.d
	rm -f xmakefile xmakefile-tmp
	cat Makefile > xmakefile-tmp
	echo '' >> xmakefile-tmp
	cat libWebAppServer_a-base64.d libWebAppServer_a-md5.d libWebAppServer_a-sha1.d libWebAppServer_a-fdThreadLauncher.d libWebAppServer_a-serverPorts.d libWebAppServer_a-WebAppConnection.d libWebAppServer_a-WebAppServer.d libWebAppServer_a-WebAppServerConfig.d libWebAppServer_a-WebServerConnectionBase.d libWebAppServer_a-WebFastCGIConnection.d libWebAppServer_a-WebSocketConnection.d >> xmakefile-tmp
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

libWebAppServer.a: libWebAppServer_a-base64.o libWebAppServer_a-md5.o libWebAppServer_a-sha1.o libWebAppServer_a-fdThreadLauncher.o libWebAppServer_a-serverPorts.o libWebAppServer_a-WebAppConnection.o libWebAppServer_a-WebAppServer.o libWebAppServer_a-WebAppServerConfig.o libWebAppServer_a-WebServerConnectionBase.o libWebAppServer_a-WebFastCGIConnection.o libWebAppServer_a-WebSocketConnection.o $(libWebAppServer_a_LDADD)
	rm -f libWebAppServer.a
	$(AR) cq libWebAppServer.a  libWebAppServer_a-base64.o libWebAppServer_a-md5.o libWebAppServer_a-sha1.o libWebAppServer_a-fdThreadLauncher.o libWebAppServer_a-serverPorts.o libWebAppServer_a-WebAppConnection.o libWebAppServer_a-WebAppServer.o libWebAppServer_a-WebAppServerConfig.o libWebAppServer_a-WebServerConnectionBase.o libWebAppServer_a-WebFastCGIConnection.o libWebAppServer_a-WebSocketConnection.o $(libWebAppServer_a_LDADD) $(libWebAppServer_a_LDFLAGS)

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

clean: 
	rm -f xmakefile *.d *.o libWebAppServer.a

