
#include "WebSocketServer.h"
#include "sha1.h"
#include "base64.h"

#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>

WebSocketServer :: WebSocketServer(void)
{
    wsfd = -1;
    pipe(closePipe);
    threadRunning = false;
}

WebSocketServer :: ~WebSocketServer(void)
{
    if (wsfd != -1)
        close(wsfd);
    close(closePipe[0]);
    close(closePipe[1]);
}

bool
WebSocketServer :: start(int port, WebSocketConnectionCallback *_cb)
{
    if (wsfd != -1)
    {
        fprintf(stderr, "WebSocketServer :: start : "
                "called when already running\n");
        stop();
    }

    cb = _cb;
    wsfd = socket(AF_INET, SOCK_STREAM, 0);
    if (wsfd < 0)
    {
        fprintf(stderr, "socket : %s\n", strerror(errno));
        return false;
    }

    int v = 1;
    setsockopt( wsfd, SOL_SOCKET, SO_REUSEADDR,
                (void*) &v, sizeof( v ));

    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_port = htons((short)port);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(wsfd, (struct sockaddr *)&sa, sizeof(sa)) < 0)
    {
        fprintf(stderr, "bind : %s\n", strerror(errno));
        return false;
    }

    listen(wsfd, 1);

    pthread_attr_t attr;
    pthread_attr_init( &attr );
    pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );
    pthread_t id;
    pthread_create(&id, &attr,
                   &server_thread_entry, (void*) this);
    pthread_attr_destroy( &attr );

    int count = 1000;
    while (threadRunning == false && count > 0)
    {
        count--;
        usleep(10000);
    }
    if (threadRunning == false)
    {
        fprintf(stderr, "thread failed to start!\n");
        return false;
    }

    return true;
}

void
WebSocketServer :: stop(void)
{
    if (threadRunning == true)
    {
        char c = 1;
        write(closePipe[1], &c, 1);

        int count = 1000;
        while (threadRunning == true && count > 0)
        {
            count--;
            usleep(10000);
        }
        if (threadRunning == true)
        {
            fprintf(stderr, "thread failed to die!\n");
            close(wsfd);
            close(closePipe[0]);
            close(closePipe[1]);
            pipe(closePipe);
        }
    }
}

//static
void *
WebSocketServer :: server_thread_entry( void * arg )
{
    WebSocketServer * obj = (WebSocketServer *) arg;
    printf("server thread started\n");
    obj->threadRunning = true;
    obj->server_thread_main();
    obj->threadRunning = false;
    printf("server thread exiting\n");
    return NULL;
}

void
WebSocketServer :: server_thread_main( void )
{
    bool done = false;
    while (!done)
    {
        fd_set rfds;
        int cc, max;

        FD_ZERO(&rfds);
        FD_SET(wsfd, &rfds);
        FD_SET(closePipe[0], &rfds);

        if (wsfd > closePipe[0])
            max = wsfd;
        else
            max = closePipe[0];
        cc = select(max+1, &rfds, NULL, NULL, NULL);
        if (cc < 0)
        {
            fprintf(stderr, "server thread: select: %s\n", 
                    strerror(errno));
            return;
        }
        if (cc == 0)
            continue;

        if (FD_ISSET(closePipe[0], &rfds))
        {
            char c;
            read(closePipe[0], &c, 1);
            done = true;
        }

        if (FD_ISSET(wsfd, &rfds))
        {
            struct sockaddr_in sa;
            socklen_t  salen = sizeof(sa);
            int newfd = accept(wsfd, (struct sockaddr *)&sa, &salen);
            if (newfd < 0)
            {
                fprintf(stderr, "accept: %s\n", strerror(errno));
                done = true;
            }
            else
            {

                printf("new connection from IP: %u.%u.%u.%u\n",
                       (sa.sin_addr.s_addr >>  0) & 0xFF,
                       (sa.sin_addr.s_addr >>  8) & 0xFF,
                       (sa.sin_addr.s_addr >> 16) & 0xFF,
                       (sa.sin_addr.s_addr >> 24) & 0xFF);

                WebSocketConnection * conn = cb->newConnection(newfd);

                pthread_attr_t attr;
                pthread_attr_init( &attr );
                pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );
                pthread_t id;
                pthread_create(&id, &attr,
                               &connection_thread_entry, (void*) conn);
                pthread_attr_destroy( &attr );
            }
        }
    }
}

//static
void *
WebSocketServer :: connection_thread_entry( void * arg )
{
    WebSocketConnection * conn = (WebSocketConnection *) arg;
    printf("connection handler thread is starting\n");
    conn->connection_thread_main();
    delete conn;
    printf("connection handler thread is dying\n");
    return NULL;
}
