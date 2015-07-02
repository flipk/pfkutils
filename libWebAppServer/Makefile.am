
lib_LTLIBRARIES = libWebAppServer.la

noinst_PROGRAMS = wsServerTest wsClientTest

libWebAppServer_ladir = ${includedir}/webappserver

libWebAppServer_la_HEADERS = \
	WebAppServer.h WebSocketClient.h WebAppMessage.h \
	CircularReader.h 

libWebAppServer_la_SOURCES = serverPorts.cc \
	WebAppConnection.cc WebAppServer.cc WebAppServerConfig.cc \
	WebServerConnectionBase.cc WebFastCGIConnection.cc \
	WebSocketConnection.cc WebSocketClient.cc

libWebAppServer_la_CXXFLAGS = -g3 -Wall -Werror
libWebAppServer_la_CFLAGS = -g3 -Wall -Werror
libWebAppServer_la_CPPFLAGS = -I$(top_srcdir)/libpfkutil

wsServerTest_SOURCES = wsServerTest.cc
wsServerTest_CXXFLAGS = -g3 -Wall -Werror
wsServerTest_CPPFLAGS = -I$(top_srcdir)/libpfkutil
wsServerTest_LDADD = libWebAppServer.la ../libpfkutil/libpfkutil.la
wsServerTest_LDFLAGS = -lpthread

wsClientTest_SOURCES = wsClientTest.cc
wsClientTest_CXXFLAGS = -g3 -Wall -Werror
wsClientTest_CPPFLAGS = -I$(top_srcdir)/libpfkutil
wsClientTest_LDADD = libWebAppServer.la ../libpfkutil/libpfkutil.la
wsClientTest_LDFLAGS = -lpthread
