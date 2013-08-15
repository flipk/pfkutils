#if 0
set -e -x
g++ -c testWebSocketServer.cc
g++ -c WebSocketConnection.cc
g++ -c WebSocketServer.cc
gcc -c sha1.c
gcc -c base64.c
g++ testWebSocketServer.o WebSocketServer.o WebSocketConnection.o sha1.o base64.o -o t -lpthread
exit 0
#endif

#include "WebSocketServer.h"

#include <stdio.h>
#include <unistd.h>

class myWebSocketConnectionCallback : public WebSocketConnectionCallback {
public:
    /*virtual*/ WebSocketConnection * newConnection(int fd);
};

int
main()
{
    myWebSocketConnectionCallback callback;
    WebSocketServer      server;

    if (server.start(1081, &callback) == false)
    {
        printf("failure to start server\n");
        return 1;
    }

    while (1)
        sleep(1);

    return 0;
}

class myWebSocketConnection : public WebSocketConnection {
public:
    myWebSocketConnection(int _fd);
    /*virtual*/ ~myWebSocketConnection(void);
    /*virtual*/ void onMessage(const WebSocketMessage &);
};

WebSocketConnection *
myWebSocketConnectionCallback :: newConnection(int fd)
{
    return new myWebSocketConnection(fd);
}

myWebSocketConnection :: myWebSocketConnection(int _fd)
    : WebSocketConnection(_fd)
{
}

myWebSocketConnection :: ~myWebSocketConnection(void)
{
}

void
myWebSocketConnection :: onMessage(const WebSocketMessage &m)
{
}
