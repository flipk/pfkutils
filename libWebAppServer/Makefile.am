
CC=gcc
CXX=g++
AR=ar

noinst_LIBRARIES = libWebAppServer.a

noinst_PROGRAMS = wsServerTest wsClientTest

libWebAppServer_a_SOURCES = base64.cc md5.cc sha1.cc \
	fdThreadLauncher.cc serverPorts.cc \
	WebAppConnection.cc WebAppServer.cc WebAppServerConfig.cc \
	WebServerConnectionBase.cc \
	WebFastCGIConnection.cc WebSocketConnection.cc \
	WebSocketClient.cc

libWebAppServer_a_HEADERS = base64.h md5.h sha1.h \
	WebAppServer.h WebAppServerInternal.h \
	WebSocketClient.h

libWebAppServer_a_CPPFLAGS = -I$(HOME)/proj/pfkutils/libpfkutil
libWebAppServer_a_CXXFLAGS = -g3 -Wall -Werror
libWebAppServer_a_CFLAGS = -g3 -Wall -Werror



wsServerTest_SOURCES = wsServerTest.cc
wsServerTest_CXXFLAGS = -g3 -Wall -Werror
wsServerTest_CPPFLAGS = -I$(HOME)/proj/pfkutils/libpfkutil
wsServerTest_LDADD = libWebAppServer.a
wsServerTest_LDFLAGS = -lpthread

wsClientTest_SOURCES = wsClientTest.cc
wsClientTest_CXXFLAGS = -g3 -Wall -Werror
wsClientTest_CPPFLAGS = -I$(HOME)/proj/pfkutils/libpfkutil
wsClientTest_LDADD = libWebAppServer.a
wsClientTest_LDFLAGS = -lpthread
