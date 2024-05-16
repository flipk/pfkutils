
#include "simpleWebSocket.h"
#ifndef DEPENDING
#include SIMPLEWSTESTMSGS_PB_H
#endif
#include <list>
#include <pthread.h>

#define DEBUG false

void *connection_thread(void*arg);

int main()
{
    ::simpleWsTest::ProxyMsg  msg;
    SimpleWebSocket::WebSocketClientConn clnt(
//        0x7f000001,1081,"/websocket/some_application"
        "ws://127.0.0.1:1081/websocket/some_application",
        DEBUG
        );

    if (clnt.ok() == false)
    {
        printf("client setup fail\n");
        return 3;
    }

    pthread_t id;
    pthread_create(&id, NULL,
                   &connection_thread, (void*) &clnt);

    bool done = false;
    while (!done)
    {
        SimpleWebSocket::WebSocketRet ret;
        ret = clnt.handle_read(msg);
        switch (ret)
        {
        case SimpleWebSocket::WEBSOCKET_CONNECTED:
            printf("WebSocket connected!\n");
            msg.Clear();
            msg.set_type(simpleWsTest::PMT_PROTOVERSION);
            msg.set_sequence(0);
            msg.mutable_protover()->set_version(1);
            clnt.sendMessage(msg);
            break;

        case SimpleWebSocket::WEBSOCKET_NO_MESSAGE:
            // go round again
            break;

        case SimpleWebSocket::WEBSOCKET_MESSAGE:
            switch (msg.type())
            {
            case simpleWsTest::PMT_PROTOVERSION:
                printf("remote proto version = %d\n",
                       msg.protover().version());
                break;

            case simpleWsTest::PMT_CLOSING:
                printf("remote side is closing, so we are too\n");
                done = true;
                break;

            case simpleWsTest::PMT_DATA:
                if (::write(1, msg.data().data().c_str(),
                            msg.data().data().size()) < 0)
                {
                    // quiet an obnoxious UB14 compiler.
                    printf("write stdout failed\n");
                }
                break;

            case simpleWsTest::PMT_PING:
                printf("got ping\n");
                break;

            default:
                printf("msg.type() = %d\n", msg.type());
            }
            break;

        case SimpleWebSocket::WEBSOCKET_CLOSED:
            printf("WebSocket closed!\n");
            done = true;
            break;
        }
    }

    printf("closing connection\n");
    return 0;
}

void *
connection_thread(void*arg)
{
    ::simpleWsTest::ProxyMsg  msg;
    SimpleWebSocket::WebSocketClientConn *clnt =
        (SimpleWebSocket::WebSocketClientConn *) arg;
    int sequence = 1;

    bool done = false;
    while (!done)
    {
        msg.Clear();
        msg.set_type(simpleWsTest::PMT_DATA);
        msg.set_sequence(sequence++);
        std::string *buf = msg.mutable_data()->mutable_data();
        buf->resize(1024);
        int cc = ::read(0, (void*) buf->c_str(), buf->length());
        if (cc > 0)
        {
            buf->resize(cc);
            clnt->sendMessage(msg);
        }
        else
        {
            msg.Clear();
            msg.set_type(simpleWsTest::PMT_CLOSING);
            msg.set_sequence(sequence++);
            clnt->sendMessage(msg);
            done = true;
        }
    }

    return NULL;
}
