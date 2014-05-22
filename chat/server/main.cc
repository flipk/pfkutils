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

int
main()
{
    pfkChatAppConnectionCallback callback;
    WebAppServerConfig  serverConfig;
    WebAppServer::WebAppServer  server;

    signal( SIGPIPE, SIG_IGN );

    initChatServer();

    serverConfig.addWebsocket(1081, "/websocket/pfkchat", &callback, 1000);
    serverConfig.addFastCGI(1082, "/cgi/pfkchat.cgi", &callback, 1000);

    if (server.start(&serverConfig) == false)
    {
        printf("failure to start server\n");
    }

    while (1)
        sleep(1);

    return 0;
}
