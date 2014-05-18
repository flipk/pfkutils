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
#include "passwordDatabase.h"

using namespace std;
using namespace PFK::Chat;
using namespace WebAppServer;

static PasswordDatabase * pwd_db;
static list<pfkChatAppConnection*> clientList;
typedef list<pfkChatAppConnection*>::iterator clientListIter_t;
static pthread_mutex_t  clientMutex;
#define   lock() pthread_mutex_lock  ( &clientMutex )
#define unlock() pthread_mutex_unlock( &clientMutex )

void
initChatServer(void)
{
    pthread_mutexattr_t  mattr;
    pthread_mutexattr_init( &mattr );
    pthread_mutex_init( &clientMutex, &mattr );
    pthread_mutexattr_destroy( &mattr );
    pwd_db = new PasswordDatabase;
}

pfkChatAppConnection :: pfkChatAppConnection(void)
{
    lock();
    clientList.push_back(this);
    unlock();
    authenticated = false;
    typing = STATE_EMPTY;
    id = -1;
    username = "guest";
    idleTime = 0;
    unreadCount = 0;
}

pfkChatAppConnection :: ~pfkChatAppConnection(void)
{
    lock();
    clientList.remove(this);
    unlock();

    if (authenticated)
    {
        ServerToClient  stc;
        stc.set_type( STC_USER_STATUS );
        stc.mutable_userstatus()->set_username(username);
        stc.mutable_userstatus()->set_status(USER_LOGGED_OUT);
        sendClientMessage(stc,true);
        sendUserList(true);
    }
}

void
pfkChatAppConnection :: sendClientMessage(const ServerToClient &outmsg,
                                           bool broadcast, bool meToo)
{
//    if (outmsg.type() != STC_PONG)
    cout << "sending message to browser: " << outmsg.DebugString() << endl;

    string  buf;    
    outmsg.SerializeToString( &buf );
    const WebAppMessage outm(WS_TYPE_BINARY, buf);

    bool publicOkay = false;
    switch (outmsg.type())
    {
    case STC_PROTOVERSION_RESP:
    case STC_LOGIN_STATUS:
    case STC_PONG:
        publicOkay = true;
        break;
    case STC_IM_MESSAGE:
    case STC_USER_LIST:
    case STC_USER_STATUS:
    default:
        publicOkay = false;
        break;
    }

    if (broadcast)
    {
        lock();
        clientListIter_t it;
        for (it = clientList.begin(); it != clientList.end(); it++)
        {
            pfkChatAppConnection * c = *it;
            if (meToo || c != this)
                if (publicOkay || c->authenticated)
                    c->sendMessage(outm);
        }
        unlock();
    }
    else
    {
        if (publicOkay || authenticated)
            sendMessage(outm);
    }
}

void
pfkChatAppConnection :: sendUserList(bool broadcast, bool meToo)
{
    ServerToClient  stc;
    stc.set_type( STC_USER_LIST );
    UserList * ul = stc.mutable_userlist();

    lock();
    clientListIter_t it;
    for (it = clientList.begin(); it != clientList.end(); it++)
    {
        pfkChatAppConnection * c = *it;
        if (c->get_authenticated())
        {
            UserInfo * ui = ul->add_users();
            ui->set_id(c->get_id());
            ui->set_username(c->get_username());
            ui->set_typing(c->get_typing());
            ui->set_idle(c->get_idleTime());
            ui->set_unread(c->get_unreadCount());
        }
    }
    unlock();

    sendClientMessage(stc,broadcast,meToo);
}

static bool
legalString( string str )
{
    int len = str.length();
    if (len > 20)
        return false;
    int good = 0;
    for (int pos = 0; pos < len; pos++)
    {
        int c = str[pos];
        if (c >= '0' && c <= '9')
            good++;
        if (c >= 'a' && c <= 'z')
            good++;
        if (c >= 'A' && c <= 'Z')
            good++;
    }
    return good == len;
}

bool
pfkChatAppConnection :: onMessage(const WebAppMessage &m)
{
    ClientToServer   msg;

    if (m.type == WS_TYPE_CLOSE)
    {
        cout << "pfkChatAppConnection got CLOSE packet!" << endl;
        return false;
    }

    if (m.type != WS_TYPE_BINARY)
    {
        cout << "pfkChatAppConnection got TEXT packet!" << endl;
        return false;
    }

    if (msg.ParseFromString(m.buf) == false)
    {
        cout << "ParseFromString failed!" << endl;
        return false;
    }

//    if (msg.type() != CTS_PING)
        cout << "decoded message from server: " << msg.DebugString() << endl;

    if (authenticated == false &&
        msg.type() != CTS_PROTOVERSION &&
        msg.type() != CTS_PING &&
        msg.type() != CTS_LOGIN &&
        msg.type() != CTS_REGISTER)
    {
        cout << "disallowed message: not authenticated" << endl;
        return false;
    }

    switch (msg.type())
    {
    case CTS_PROTOVERSION:
    {
        ServerToClient  srv2cli;
        srv2cli.set_type( STC_PROTOVERSION_RESP );
        if (msg.has_protoversion() && msg.protoversion().version() ==
            PFK_CHAT_CurrentProtoVersion)
        {
            srv2cli.set_protoversionresp(PROTO_VERSION_MATCH);
        }
        else
        {
            srv2cli.set_protoversionresp(PROTO_VERSION_MISMATCH);
        }
        sendClientMessage( srv2cli, false );
        break;
    }
    case CTS_PING:
    {
        ServerToClient  srv2cli;
        srv2cli.set_type( STC_PONG );
        sendClientMessage( srv2cli, false );

        if (msg.ping().has_idle())
            idleTime = msg.ping().idle();
        else
            idleTime = 0;

        if (msg.ping().has_unread())
            unreadCount = msg.ping().unread();
        else
            unreadCount = 0;

        if (authenticated)
        {
            if (msg.ping().has_forced() && msg.ping().forced())
                sendUserList(true);
            else
                sendUserList(false);
        }
        break;
    }
    case CTS_LOGIN:
    {
        PasswordEntry * pw = pwd_db->lookupUser(msg.login().username());
        ServerToClient  srv2cli;
        srv2cli.set_type( STC_LOGIN_STATUS );

        if (pw == NULL)
        {
            srv2cli.mutable_loginstatus()->set_status( LOGIN_REJECT );
        }
        else
        {
            if (msg.login().has_password())
            {
                if (pw->password == msg.login().password())
                {
                    pwd_db->newToken(pw);
                    srv2cli.mutable_loginstatus()->set_status( LOGIN_ACCEPT );
                    srv2cli.mutable_loginstatus()->set_token( pw->token );
                    id = pw->id;
                    srv2cli.mutable_loginstatus()->set_id( id );
                    username = pw->username;
                    authenticated = true;
                    ServerToClient  stc;
                    stc.set_type( STC_USER_STATUS );
                    stc.mutable_userstatus()->set_username(username);
                    stc.mutable_userstatus()->set_status(USER_LOGGED_IN);
                    sendClientMessage(stc,true);
                    sendUserList(true);
                }
                else
                {
                    srv2cli.mutable_loginstatus()->set_status( LOGIN_REJECT );
                }
            }
            else if (msg.login().has_token())
            {
                if (pw->token == "__INVALID__" ||
                    pw->token != msg.login().token())
                {
                    srv2cli.mutable_loginstatus()->set_status( LOGIN_REJECT );
                }
                else
                {
                    srv2cli.mutable_loginstatus()->set_status( LOGIN_ACCEPT );
                    id = pw->id;
                    srv2cli.mutable_loginstatus()->set_id( id );
                    username = pw->username;
                    authenticated = true;
                    ServerToClient  stc;
                    stc.set_type( STC_USER_STATUS );
                    stc.mutable_userstatus()->set_username(username);
                    stc.mutable_userstatus()->set_status(USER_LOGGED_IN);
                    sendClientMessage(stc,true);
                    sendUserList(true);
                }
            }
        }
        sendClientMessage( srv2cli, false );
        break;
    }
    case CTS_REGISTER:
    {
        string uname = msg.login().username();
        string pwd   = msg.login().password();
        ServerToClient  srv2cli;
        srv2cli.set_type( STC_LOGIN_STATUS );

        if (!legalString(uname))
        {
            srv2cli.mutable_loginstatus()->set_status(
                REGISTER_INVALID_USERNAME );
        }
        else
        {
            if (!legalString(pwd))
            {
                srv2cli.mutable_loginstatus()->set_status(
                    REGISTER_INVALID_PASSWORD );
            }
            else
            {
                PasswordEntry * ent = pwd_db->lookupUser(uname);
                if (ent != NULL)
                {
                    srv2cli.mutable_loginstatus()->set_status(
                        REGISTER_DUPLICATE_USERNAME );
                }
                else
                {
                    ent = pwd_db->addUser(uname, pwd);
                    if (ent == NULL)
                    {
                        srv2cli.mutable_loginstatus()->set_status(
                            REGISTER_INVALID_USERNAME );
                    }
                    else
                    {
                        srv2cli.mutable_loginstatus()->set_status(
                            REGISTER_ACCEPT );
                        srv2cli.mutable_loginstatus()->set_token(
                            ent->token );
                        id = ent->id;
                        srv2cli.mutable_loginstatus()->set_id( id );
                        username = ent->username;
                        authenticated = true;
                        ServerToClient  stc;
                        stc.set_type( STC_USER_STATUS );
                        stc.mutable_userstatus()->set_username(username);
                        stc.mutable_userstatus()->set_status(USER_LOGGED_IN);
                        sendClientMessage(stc,true);
                        sendUserList(true);
                    }
                }
            }
        }
        sendClientMessage( srv2cli, false );
        break;
    }
    case CTS_LOGOUT:
    {
        authenticated = false;
        id = -1;
        PasswordEntry * ent = pwd_db->lookupUser(username);
        if (ent)
        {
            ent->token = "__INVALID__";
            pwd_db->sync();
            ServerToClient  stc;
            stc.set_type( STC_USER_STATUS );
            stc.mutable_userstatus()->set_username(username);
            stc.mutable_userstatus()->set_status(USER_LOGGED_OUT);
            sendClientMessage(stc,true);
            sendUserList(true);
        }
        break;
    }
    case CTS_IM_MESSAGE:
    {
        ServerToClient  srv2cli;
        srv2cli.set_type( STC_IM_MESSAGE );
        srv2cli.mutable_im()->CopyFrom(msg.im());
        srv2cli.mutable_im()->set_username(username);
        sendClientMessage( srv2cli, true, false );
        break;
    }
    case CTS_TYPING_IND:
    {
        idleTime = 0;
        typing = msg.typing().state();
        sendUserList(true);
        break;
    }
    default:
        break;
    }
    return true;
}

bool
pfkChatAppConnection :: doPoll(void)
{
//    cout << "chat app do poll!" << endl;
    return true;
}
