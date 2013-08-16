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
#include <pthread.h>

class myWebSocketConnectionCallback : public WebSocketConnectionCallback {
public:
    /*virtual*/ WebSocketConnection * newConnection(int fd);
};

static void initClientList(void);

int
main()
{
    myWebSocketConnectionCallback callback;
    WebSocketServer      server;

    initClientList();
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
    class myWebSocketConnection * next;
    class myWebSocketConnection * prev;
    char username[128];
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

myWebSocketConnection * clientList;
pthread_mutex_t  clientMutex;
#define   lock() pthread_mutex_lock  ( &clientMutex )
#define unlock() pthread_mutex_unlock( &clientMutex )

static void
initClientList(void)
{
    clientList = NULL;
    pthread_mutexattr_t  mattr;
    pthread_mutexattr_init( &mattr );
    pthread_mutex_init( &clientMutex, &mattr );
    pthread_mutexattr_destroy( &mattr );
}

myWebSocketConnection :: myWebSocketConnection(int _fd)
    : WebSocketConnection(_fd)
{
    lock();
    if (clientList)
        clientList->prev = this;
    next = clientList;
    prev = NULL;
    clientList = this;
    unlock();
}

myWebSocketConnection :: ~myWebSocketConnection(void)
{
    lock();
    if (prev)
        prev->next = next;
    else
        clientList = next;
    if (next)
        next->prev = prev;
    unlock();
}

void
myWebSocketConnection :: onMessage(const WebSocketMessage &m)
{
//    WebSocketMessage outm;
//    outm.type = WS_TYPE_TEXT;
//    outm.buf = (uint8_t*)"user SHITFUCK logged in";
//    outm.len = sizeof("user SHITFUCK logged in");
//    sendMessage(outm);
}
