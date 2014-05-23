
noinst_LIBRARIES = libWebAppServer.a

libWebAppServer_a_SOURCES = base64.cc md5.cc sha1.cc \
	fdThreadLauncher.cc serverPorts.cc \
	WebAppConnection.cc WebAppServer.cc WebAppServerConfig.cc \
	WebServerConnectionBase.cc \
	WebFastCGIConnection.cc WebSocketConnection.cc

libWebAppServer_a_HEADERS = base64.h md5.h sha1.h \
	WebAppServer.h WebAppServerInternal.h

libWebAppServer_a_CXXFLAGS = -g3 -Wall -Werror
libWebAppServer_a_CFLAGS = -g3 -Wall -Werror
