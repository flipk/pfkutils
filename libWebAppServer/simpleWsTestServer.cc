
#include "simpleWebSocket.h"
#include PROXYMSGS_PB_H
#include <map>
#include <pthread.h>

#define DEBUG false

void *connection_thread(void*arg);

int main()
{
    SimpleWebSocket::WebSocketServer  srv(1081, INADDR_ANY, DEBUG);

    if (srv.ok() == false)
    {
        printf("server setup fail\n");
        return 3;
    }

    while (1)
    {
        SimpleWebSocket::WebSocketServerConn *nc = srv.handle_accept();
        if (nc)
        {
            pthread_t id;
            pthread_create(&id, NULL,
                           &connection_thread, (void*) nc);
        }
    }

    return 0;
}

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int conns_pos = 1;
std::map<int,SimpleWebSocket::WebSocketServerConn *> conns;

void *connection_thread(void*arg)
{
    ::proxyTcp::ProxyMsg  msg;
    SimpleWebSocket::WebSocketServerConn *nc =
        (SimpleWebSocket::WebSocketServerConn *) arg;

    printf("got connection\n");
    pthread_mutex_lock(&mutex);
    int pos = conns_pos++;
    conns[pos] = nc;
    pthread_mutex_unlock(&mutex);

    bool done = false;
    while (!done)
    {
        SimpleWebSocket::WebSocketRet ret;
        ret = nc->handle_read(msg);
        switch (ret)
        {
        case SimpleWebSocket::WEBSOCKET_CONNECTED:
        {
            std::string path;
            nc->get_path(path);
            printf("WebSocket connected! path = '%s'\n",
                   path.c_str());
            msg.Clear();
            msg.set_type(proxyTcp::PMT_PROTOVERSION);
            msg.set_sequence(0);
            msg.mutable_protover()->set_version(1);
            nc->sendMessage(msg);
            break;
        }
        case SimpleWebSocket::WEBSOCKET_NO_MESSAGE:
            // go round again
            break;

        case SimpleWebSocket::WEBSOCKET_MESSAGE:
            switch (msg.type())
            {
            case proxyTcp::PMT_PROTOVERSION:
                printf("remote proto version = %d\n",
                       msg.protover().version());
                break;

            case proxyTcp::PMT_CLOSING:
                printf("remote side is closing, so we are too\n");
                done = true;
                break;

            case proxyTcp::PMT_DATA:
            {
                printf("got data length %d\n",
                       msg.data().data().size());
                pthread_mutex_lock(&mutex);
                for (int ind = 0; ind < conns_pos; ind++)
                    if (ind != pos && conns[ind] != NULL)
                    {
                        conns[ind]->sendMessage(msg);
                    }
                pthread_mutex_unlock(&mutex);
                break;
            }
            case proxyTcp::PMT_PING:
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

    pthread_mutex_lock(&mutex);
    conns[pos] = NULL;
    pthread_mutex_unlock(&mutex);

    delete nc;
    return NULL;
}
