#if 0
set -e -x

node="/home/flipk/proj/web_servers/installed/nodejs"
proto2js="$node/lib/node_modules/protobufjs/bin/proto2js"
proto="/home/flipk/proj/web_servers/installed/protobuf"
protoc="$proto/bin/protoc"
flags="-Wall -Werror -O6"
incs="-I$proto/include"
libs="-L$proto/lib -lprotobuf -lpthread"

$protoc --cpp_out=. pfkchat.proto
$proto2js pfkchat.proto -class > pfkchat.js

g++ $incs $flags -c testWebSocketServer.cc
g++ $incs $flags -c WebSocketConnection.cc
g++ $incs $flags -c WebSocketServer.cc
g++ $incs $flags -c pfkchat.pb.cc
gcc $incs $flags -c sha1.c
gcc              -c md5.c
gcc $incs $flags -c base64.c
g++ testWebSocketServer.o WebSocketServer.o WebSocketConnection.o pfkchat.pb.o sha1.o base64.o md5.o $libs -o t

LD_LIBRARY_PATH=$proto/lib ./t

exit 0
#endif

#include "WebSocketServer.h"

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>

#include <iostream>

#include "pfkchat.pb.h"
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

    signal( SIGPIPE, SIG_IGN );

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

    srv2cli.set_type( STC_LOGOUT_NOTIFICATION );
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

    WebSocketMessage outm;
    outm.type = WS_TYPE_BINARY;
    outm.buf = (uint8_t*)  outmsgbinary.data();
    outm.len = (int)       outmsgbinary.length();

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

    srv2cli.set_type( STC_USER_LIST );
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
    ClientToServer   msg;

    if (msg.ParseFromString(string((char*)m.buf, (int)m.len)) == false)
    {
        cout << "ParseFromString failed!" << endl;
        return;
    }

    cout << "decoded message from server: " << msg.DebugString() << endl;

    switch (msg.type())
    {
    case CTS_PING:
    {
        printf("user %s sent ping\n", username);
        ServerToClient  srv2cli;
        srv2cli.set_type( STC_PONG );
        sendClientMessage( srv2cli, false );
        break;
    }

    case CTS_LOGIN:
    {
        strcpy(username, msg.login().username().c_str());
        printf("user %s has logged in\n", username);

        ServerToClient  srv2cli;
        srv2cli.set_type( STC_LOGIN_NOTIFICATION );
        srv2cli.mutable_notification()->set_username( username );
        sendClientMessage( srv2cli, true );
        break;
    }

    case CTS_CHANGE_USERNAME:
    {
        strcpy(username, msg.changeusername().newusername().c_str());
        printf("user %s changed their username to %s\n",
               msg.changeusername().oldusername().c_str(),
               username);

        ServerToClient  outmsg;
        outmsg.set_type( STC_CHANGE_USERNAME );
        outmsg.mutable_changeusername()->CopyFrom( msg.changeusername() );
        sendClientMessage( outmsg, true );
        break;
    }

    case CTS_IM_MESSAGE:
    {
        ServerToClient  outmsg;
        outmsg.set_type( STC_IM_MESSAGE );
        outmsg.mutable_immessage()->CopyFrom( msg.immessage() );
        sendClientMessage( outmsg, true );
        break;
    }
    }
}
