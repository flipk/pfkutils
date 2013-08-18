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
    void sendClientMessage(const ServerToClient &msg, bool broadcast);
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
    ServerToClient  srv2cli;

    srv2cli.set_type( ServerToClient_ServerToClientType_LOGOUT_NOTIFICATION );
    srv2cli.mutable_notification()->set_username( username );

    sendClientMessage( srv2cli, true );

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
myWebSocketConnection :: sendClientMessage(const ServerToClient &outmsg,
                                           bool broadcast)
{
    string outmsgbinary;

    outmsg.SerializeToString( &outmsgbinary );

    uint8_t outb64buf[1024];
    int inlen = outmsgbinary.length();
    int inpos, outpos;
    for (inpos = 0, outpos = 0; inpos < inlen; inpos += 3, outpos += 4)
    {
        int encodelen = inlen - inpos;
        if (encodelen > 3)
            encodelen = 3;

        uint8_t  in3buf[3];
        in3buf[0] = outmsgbinary[inpos+0];
        in3buf[1] = outmsgbinary[inpos+1];
        in3buf[2] = outmsgbinary[inpos+2];

        b64_encode_quantum(in3buf, encodelen, outb64buf + outpos);
    }

    WebSocketMessage outm;
    outm.type = WS_TYPE_TEXT;
    outm.buf = outb64buf;
    outm.len = outpos;

    if (broadcast)
    {
        lock();
        for (myWebSocketConnection * c = clientList; c; c = c->next)
        {
            c->sendMessage(outm);
        }
        unlock();
    }
    else
    {
        sendMessage(outm);
    }
}

void
myWebSocketConnection :: onReady(void)
{

    ServerToClient  srv2cli;

    srv2cli.set_type( ServerToClient_ServerToClientType_USER_LIST );
    UserList * ul = srv2cli.mutable_userlist();

    lock();
    for (myWebSocketConnection * c = clientList; c; c = c->next)
        if (c != this)
            ul->add_usernames(c->username);
    unlock();

    sendClientMessage( srv2cli, false );
}

void
myWebSocketConnection :: onMessage(const WebSocketMessage &m)
{
    int cc, inpos, outpos, newlen;
    string binaryBuffer;

    // rewrite m.buf in place stripping \r and \n
    for (inpos = 0, outpos = 0; inpos < m.len; inpos++)
    {
        uint8_t c = m.buf[inpos];
        if (c != 13 && c != 10)
            m.buf[outpos++] = c;
    }
    newlen = outpos;

    outpos = 0;
    for (inpos = 0; inpos < newlen; inpos += 4)
    {
        unsigned char output[3];
        cc = b64_decode_quantum(m.buf + inpos, output);
        if (cc == 0)
        {
            printf("bogus base64 decode, bailing\n");
            return;
        }
        binaryBuffer.push_back(output[0]);
        binaryBuffer.push_back(output[1]);
        binaryBuffer.push_back(output[2]);
        outpos += cc;
    }

    ClientToServer   msg;

    msg.ParseFromString(binaryBuffer);

//    cout << "decoded message from server: " << msg.DebugString() << endl;

    switch (msg.type())
    {
    case ClientToServer_ClientToServerType_PING:
    {
        printf("user %s sent ping\n", username);
        ServerToClient  srv2cli;
        srv2cli.set_type( ServerToClient_ServerToClientType_PONG );
        sendClientMessage( srv2cli, false );
        break;
    }

    case ClientToServer_ClientToServerType_LOGIN:
    {
        strcpy(username, msg.login().username().c_str());
        printf("user %s has logged in\n", username);

        ServerToClient  srv2cli;
        srv2cli.set_type(
            ServerToClient_ServerToClientType_LOGIN_NOTIFICATION );
        srv2cli.mutable_notification()->set_username( username );
        sendClientMessage( srv2cli, true );
        break;
    }

    case ClientToServer_ClientToServerType_CHANGE_USERNAME:
    {
        strcpy(username, msg.changeusername().newusername().c_str());
        printf("user %s changed their username to %s\n",
               msg.changeusername().oldusername().c_str(),
               username);

        ServerToClient  outmsg;
        outmsg.set_type( ServerToClient_ServerToClientType_CHANGE_USERNAME );
        outmsg.mutable_changeusername()->CopyFrom( msg.changeusername() );
        sendClientMessage( outmsg, true );
        break;
    }

    case ClientToServer_ClientToServerType_IM_MESSAGE:
    {
        ServerToClient  outmsg;
        outmsg.set_type( ServerToClient_ServerToClientType_IM_MESSAGE );
        outmsg.mutable_immessage()->CopyFrom( msg.immessage() );
        sendClientMessage( outmsg, true );
        break;
    }
    }
}
