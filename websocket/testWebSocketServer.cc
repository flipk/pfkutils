#if 0
set -e -x

proto="/home/flipk/proj/web_servers/installed/protobuf"
flags="-Wall -Werror -O6"
incs="-I$proto/include"
libs="-L$proto/lib -lprotobuf -lpthread"

../../web_servers/installed/protobuf/bin/protoc --cpp_out=. pfkchat.pbj

/home/flipk/proj/web_servers/protojs/pbj pfkchat.pbj pfkchat.pbj.js

g++ $incs $flags -c testWebSocketServer.cc
g++ $incs $flags -c WebSocketConnection.cc
g++ $incs $flags -c WebSocketServer.cc
g++ $incs $flags -c pfkchat.pbj.pb.cc
gcc $incs $flags -c sha1.c
gcc $incs $flags -c base64.c
g++ testWebSocketServer.o WebSocketServer.o WebSocketConnection.o pfkchat.pbj.pb.o sha1.o base64.o $libs -o t

LD_LIBRARY_PATH=$proto/lib ./t

exit 0
#endif

#include "WebSocketServer.h"

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#include <iostream>

#include "pfkchat.pbj.pb.h"
#include "base64.h"

class myWebSocketConnectionCallback : public WebSocketConnectionCallback {
public:
    /*virtual*/ WebSocketConnection * newConnection(int fd);
};

static void initClientList(void);

using namespace std;
using namespace PFK::Chat;


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
        sendMessage(outm);
    }
    unlock();
}

void
myWebSocketConnection :: onMessage(const WebSocketMessage &m)
{
    int cc, inpos, outpos;
    string binaryBuffer;

    outpos = 0;
    for (inpos = 0; inpos < m.len; inpos += 4)
    {
        unsigned char output[3];
        cc = b64_decode_quantum(m.buf + inpos, output);
        if (cc == 0)
        {
            printf("bogus base64 decode, bailing\n");
            return;
        }
//        printf("%u %u %u ", output[0], output[1], output[2]);
        binaryBuffer.push_back(output[0]);
        binaryBuffer.push_back(output[1]);
        binaryBuffer.push_back(output[2]);
        outpos += cc;
    }
//    printf("base64 complete with %d bytes\n", outpos);

    ClientToServer   msg;

    msg.ParseFromString(binaryBuffer);

//    cout << "decoded message from server: " << msg.DebugString() << endl;

    switch (msg.type())
    {
    case ClientToServer_ClientToServerType_PING:
        printf("user %s sent ping\n", username);
        break;
    case ClientToServer_ClientToServerType_LOGIN:
    {
        strcpy(username, msg.login().username().c_str());
        printf("user %s has logged in\n", username);

        WebSocketMessage outm;
        outm.type = WS_TYPE_TEXT;
        char outmbuf[1024];
        outm.buf = (uint8_t*) outmbuf;
        outm.len = sprintf(
            outmbuf, "user %s has logged in", username);

        lock();
        for (myWebSocketConnection * c = clientList; c; c = c->next)
        {
            c->sendMessage(outm);
        }
        unlock();

        break;
    }
    case ClientToServer_ClientToServerType_CHANGE_USERNAME:
    {
        strcpy(username, msg.changeusername().newusername().c_str());
        printf("user %s changed their username to %s\n",
               msg.changeusername().oldusername().c_str(),
               username);

        WebSocketMessage outm;
        outm.type = WS_TYPE_TEXT;
        char outmbuf[1024];
        outm.buf = (uint8_t*) outmbuf;
        outm.len = sprintf(
            outmbuf, "user %s changed their username to %s",
            msg.changeusername().oldusername().c_str(),
            username);

        lock();
        for (myWebSocketConnection * c = clientList; c; c = c->next)
        {
            c->sendMessage(outm);
        }
        unlock();

        break;
    }
    case ClientToServer_ClientToServerType_IM_MESSAGE:
    {
        WebSocketMessage outm;
        outm.type = WS_TYPE_TEXT;
        char outmbuf[1024];
        outm.buf = (uint8_t*) outmbuf;
        outm.len = sprintf(
            outmbuf, "%s:%s", 
            msg.immessage().username().c_str(),
            msg.immessage().msg().c_str());

        lock();
        for (myWebSocketConnection * c = clientList; c; c = c->next)
        {
            c->sendMessage(outm);
        }
        unlock();
        break;
    }
    }


}
