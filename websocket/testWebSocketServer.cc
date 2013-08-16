#if 0
set -e -x
g++ -Wall -Werror -O6 -c testWebSocketServer.cc
g++ -Wall -Werror -O6 -c WebSocketConnection.cc
g++ -Wall -Werror -O6 -c WebSocketServer.cc
gcc -Wall -Werror -O6 -c sha1.c
gcc -Wall -Werror -O6 -c base64.c
g++ testWebSocketServer.o WebSocketServer.o WebSocketConnection.o sha1.o base64.o -o t -lpthread
exit 0
#endif

#include "WebSocketServer.h"

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

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
    /*virtual*/ void onReady(void);
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
    strcpy(username, "guest");
}

myWebSocketConnection :: ~myWebSocketConnection(void)
{
    WebSocketMessage outm;
    char text[128];
    outm.type = WS_TYPE_TEXT;
    outm.buf = (uint8_t*)text;

    outm.len = sprintf(text,"-->user %s logged out", username);

    lock();
    for (myWebSocketConnection * c = clientList; c; c = c->next)
    {
        c->sendMessage(outm);
    }
    unlock();

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
myWebSocketConnection :: onReady(void)
{
    WebSocketMessage outm;
    char text[128];
    outm.type = WS_TYPE_TEXT;
    outm.buf = (uint8_t*)text;

    outm.len = sprintf(text,"Users logged in:");
    sendMessage(outm);

    lock();
    for (myWebSocketConnection * c = clientList; c; c = c->next)
    {
        outm.len = sprintf(text,"-->%s", c->username);
        c->sendMessage(outm);
    }
    unlock();
}

void
myWebSocketConnection :: onMessage(const WebSocketMessage &m)
{
    if (memcmp(m.buf, "__PINGPONG", 10) == 0)
    {
        printf("user %s sent ping\n", username);
        return;
    }
    if (memcmp(m.buf, "__USERLOGIN:", 12) == 0)
    {
        memset(username, 0, sizeof(username));
        memcpy(username, m.buf+12, m.len-12);
        printf("username changed to %s\n", username);
        return;
    }
    lock();
    for (myWebSocketConnection * c = clientList; c; c = c->next)
    {
        c->sendMessage(m);
    }
    unlock();
}
