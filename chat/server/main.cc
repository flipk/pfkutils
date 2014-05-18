/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#include "WebAppServer.h"

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>

#include <iostream>

#include "pfkchat-messages.pb.h"
#include "pfkchat-protoversion.h"

#include "chatApp.h"

using namespace std;
using namespace PFK::Chat;
using namespace WebAppServer;

class cgiTestAppConn : public WebAppConnection {
public:
    cgiTestAppConn(void) {
        cout << "new cgi test app connection" << endl;
    }
    /*virtual*/ ~cgiTestAppConn(void) {
        cout << "cgi test app destroyed" << endl;
    }
    /*virtual*/ bool onMessage(const WebAppMessage &m) {
        cout << "got msg: " << m.buf << endl;
        return true;
    }
    /*virtual*/ bool doPoll(void) {
        cout << "test app doPoll" << endl;
        return true;
    }
};

class cgiTestAppCallback : public WebAppConnectionCallback {
public:
    cgiTestAppCallback(void) {
        cout << "cgiTestAppCallback constructor" << endl;
    }
    /*virtual*/ ~cgiTestAppCallback(void) {
        cout << "cgiTestAppCallback destructor" << endl;
    }
    /*virtual*/ WebAppConnection * newConnection(void) {
        return new cgiTestAppConn;
    }
};

int
main()
{
    pfkChatAppConnectionCallback callback;
    WebAppServerConfig  serverConfig;
    WebAppServer::WebAppServer  server;

    cgiTestAppCallback testAppCallback;

    signal( SIGPIPE, SIG_IGN );

    initChatServer();

    serverConfig.addWebsocket(1081, "/websocket/pfkchat", &callback, 1000);

    serverConfig.addFastCGI(1082, "/cgi-bin/thingy.cgi",
                            &testAppCallback, 1000);

#if 0
    serverConfig.addFastCGI(1082, "/cgi-bin/pfkchat.cgi", &callback);
    serverConfig.addFastCGI(1083, "/cgi-bin/leviathan.cgi", &callback);
    serverConfig.addFastCGI(1082, "/cgi-bin/test.cgi", &callback);
#endif

    if (server.start(&serverConfig) == false)
    {
        printf("failure to start server\n");
    }

    while (1)
        sleep(1);

    return 0;
}
