
#include "simpleWebSocket.h"
#include "posix_fe.h"
#include PROXYMSGS_PB_H

#include <list>

using namespace std;

// usage:
//     ./obj/simpleWebSocketTest s
//     ./obj/simpleWebSocketTest c

int server(void)
{
    SimpleWebSocket::WebSocketServer  srv(1081);

    if (srv.ok() == false)
    {
        printf("server setup fail\n");
        return 3;
    }

    int seq = 1;
    pxfe_poll  p;
    p.set(srv.get_fd(), POLLIN);
    p.set(0, POLLIN);
    ::proxyTcp::ProxyMsg  msg;
    list<SimpleWebSocket::WebSocketServerConn*>  conns;
    list<SimpleWebSocket::WebSocketServerConn*>::iterator cit, cot;

    bool done = false;
    while (!done)
    {
        int cc = p.poll(1000);
        if (cc <= 0)
        {
            printf("waiting\n");
        }
        if (p.rget(srv.get_fd()) & POLLIN)
        {
            SimpleWebSocket::WebSocketServerConn *nc = srv.handle_accept();
            if (nc)
            {
                conns.push_back(nc);
                p.set(nc->get_fd(), POLLIN);
            }
        }
#define POLLERRS (POLLERR | POLLHUP | POLLNVAL)
        int evt = p.rget(0);
        if (evt & POLLIN)
        {
            msg.Clear();
            msg.set_type(::proxyTcp::PMT_DATA);
            msg.set_sequence(seq++);
            string * s = msg.mutable_data()->mutable_data();
            s->resize(1000);
            int cc = read(0, (void*) s->c_str(), s->length());
            if (cc <= 0)
                done = true;
            else
            {
                s->resize(cc);
                for (cit = conns.begin(); cit != conns.end(); cit++)
                {
                    typeof(*cit) c = *cit;
                    c->sendMessage(msg);
                }
            }
        }
        if (evt & POLLERRS)
        {
            done = true;
        }
        for (cit = conns.begin(); cit != conns.end(); )
        {
            bool erased = false;
            bool closeit = false;
            typeof(*cit) c = *cit;
            evt = p.rget(c->get_fd());
            if (evt & POLLIN)
            {
                string path;
                bool readmore = true;
                while (readmore)
                {
                    switch (c->handle_read(msg))
                    {
                    case SimpleWebSocket::WEBSOCKET_CONNECTED:
                        c->get_path(path);
                        printf("CONNECTED! path is '%s'\n", path.c_str());
                        msg.Clear();
                        msg.set_type(::proxyTcp::PMT_PROTOVERSION);
                        msg.set_sequence(seq++);
                        msg.mutable_protover()->set_version(
                            ::proxyTcp::PMT_PROTO_VERSION_NUMBER);
                        c->sendMessage(msg);
                        break;

                    case SimpleWebSocket::WEBSOCKET_NO_MESSAGE:
                        printf("got NO_MESSAGE\n");
                        readmore = false;
                        break;

                    case SimpleWebSocket::WEBSOCKET_MESSAGE:
                        printf("got msg: %s\n", msg.DebugString().c_str());
                        for (cot = conns.begin();
                             cot != conns.end();
                             cot++)
                        {
                            typeof(*cot) co = *cot;
                            if (co != c)
                                co->sendMessage(msg);
                        }
                        break;

                    case SimpleWebSocket::WEBSOCKET_CLOSED:
                        printf("CLOSED!\n");
                        closeit = true;
                        readmore = false;
                        break;
                    }
                }
            }
            if (evt & POLLERRS)
            {
                closeit = true;
            }
            if (closeit)
            {
                p.set(c->get_fd(), 0);
                cit = conns.erase(cit);
                delete c;
                erased = true;
            }
            if (!erased)
                cit++;
        }
    }

    return 0;
}

int client(void)
{
    SimpleWebSocket::WebSocketClientConn clnt(
//        0x7f000001,1081,"/fuckit"
        "ws://127.0.0.1:1081/fuckit"
        );

    if (clnt.ok() == false)
    {
        printf("client setup fail\n");
        return 5;
    }

    pxfe_poll  p;
    p.set(clnt.get_fd(), POLLIN);
    p.set(0, POLLIN);
    ::proxyTcp::ProxyMsg  msg;
    int seq = 1;

    bool done = false;
    while (!done)
    {
        int cc = p.poll(1000);
        if (cc <= 0)
        {
            printf("waiting\n");
        }
        if (p.rget(0) & POLLIN)
        {
            msg.Clear();
            msg.set_type(::proxyTcp::PMT_DATA);
            msg.set_sequence(seq++);
            string * s = msg.mutable_data()->mutable_data();
            s->resize(1000);
            int cc = read(0, (void*) s->c_str(), s->length());
            if (cc <= 0)
                done = true;
            else
            {
                s->resize(cc);
                clnt.sendMessage(msg);
            }
        }
        if (p.rget(clnt.get_fd()) & POLLIN)
        {
            bool readmore = true;
            while (readmore)
            {
                switch (clnt.handle_read(msg))
                {
                case SimpleWebSocket::WEBSOCKET_CONNECTED:
                    printf("CONNECTED!\n");
                    msg.Clear();
                    msg.set_type(::proxyTcp::PMT_PROTOVERSION);
                    msg.set_sequence(seq++);
                    msg.mutable_protover()->set_version(
                        ::proxyTcp::PMT_PROTO_VERSION_NUMBER);
                    clnt.sendMessage(msg);                    break;

                case SimpleWebSocket::WEBSOCKET_NO_MESSAGE:
                    printf("got NO_MESSAGE\n");
                    readmore = false;
                    break;

                case SimpleWebSocket::WEBSOCKET_MESSAGE:
                    printf("got msg: %s\n", msg.DebugString().c_str());
                    break;

                case SimpleWebSocket::WEBSOCKET_CLOSED:
                    printf("CLOSED!\n");
                    p.set(clnt.get_fd(), 0);
                    done = true;
                    readmore = false;
                    break;
                }
            }
        }
    }

    return 0;
}

int
main(int argc, char ** argv)
{
    if (argc < 2)
        return 1;

    if (argv[1][0] == 's')
        return server();
    else if (argv[1][0] == 'c')
        return client();
    // else
    return 2;
}
