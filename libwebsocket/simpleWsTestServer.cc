
#include "simpleWebSocket.h"
#ifndef DEPENDING
#include SIMPLEWSTESTMSGS_PB_H
#endif
#include <map>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>

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
    ::simpleWsTest::ProxyMsg  msg;
    SimpleWebSocket::WebSocketServerConn *nc =
        (SimpleWebSocket::WebSocketServerConn *) arg;

    printf("got connection\n");
    pthread_mutex_lock(&mutex);
    int pos = conns_pos++;
    conns[pos] = nc;
    pthread_mutex_unlock(&mutex);

    int fd = nc->get_fd();
    fcntl( fd, F_SETFL,
           fcntl( fd, F_GETFL, 0 ) | O_NONBLOCK );
    bool doselect = false;

    bool done = false;
    while (!done)
    {
        if (doselect) {
            bool doservice = false;
            while (!doservice) {
                fd_set rfds;
                struct timeval tv;
                FD_ZERO(&rfds);
                FD_SET(fd, &rfds);
                tv.tv_sec = 1; // 1 second polling
                tv.tv_usec = 0;
                (void) select(fd+1, &rfds, NULL, NULL, &tv);
                if (FD_ISSET(fd, &rfds))
                    doservice = true;
            }
        }

        SimpleWebSocket::WebSocketRet ret = nc->handle_read(msg);
        switch (ret)
        {
        case SimpleWebSocket::WEBSOCKET_CONNECTED:
        {
            std::string path;
            doselect = false;
            nc->get_path(path);
            printf("WebSocket connected! path = '%s'\n",
                   path.c_str());
            msg.Clear();
            msg.set_type(simpleWsTest::PMT_PROTOVERSION);
            msg.set_sequence(0);
            msg.mutable_protover()->set_version(1);
            nc->sendMessage(msg);
            break;
        }
        case SimpleWebSocket::WEBSOCKET_NO_MESSAGE:
            // go round again
            doselect = true;
            break;

        case SimpleWebSocket::WEBSOCKET_MESSAGE:
            doselect = false;
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
            {
                printf("got data length %d\n",
                       (int) msg.data().data().size());
                pthread_mutex_lock(&mutex);
                for (int ind = 0; ind < conns_pos; ind++)
                    if (ind != pos && conns[ind] != NULL)
                    {
                        conns[ind]->sendMessage(msg);
                    }
                pthread_mutex_unlock(&mutex);
                break;
            }
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

    pthread_mutex_lock(&mutex);
    conns[pos] = NULL;
    pthread_mutex_unlock(&mutex);

    delete nc;
    return NULL;
}
