/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/

#include "pk_threads.h"
#include "pk_messages.h"
#include "pk_messages_ext.h"

#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

PK_Message_Ext_Link_TCP :: PK_Message_Ext_Link_TCP(
    PK_Message_Ext_Handler * _handler,
    short port)
    : PK_Message_Ext_Link(_handler),
      PK_Message_Ext_Manager(_handler,this)
{
    fd = socket( AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd < 0)
    {
        int err = errno;
        fprintf( stderr, "socket: %s\n", strerror( err ));
        return;
    }

    int v = 1;
    setsockopt( fd, SOL_SOCKET, SO_REUSEADDR, (void*) &v, sizeof( v ));
    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_port = htons( port );
    sa.sin_addr.s_addr = INADDR_ANY;
    if (bind( fd, (struct sockaddr *) &sa, sizeof( sa )) < 0)
    {
        int err = errno;
        fprintf( stderr, "bind: %s\n", strerror( err ));
        close(fd);
        fd = -1;
        return;
    }
    listen(fd, 1);
    socklen_t salen = sizeof(sa);
    int new_fd = accept4( fd, (struct sockaddr *) &sa, &salen, SOCK_CLOEXEC );
    if (new_fd < 0)
    {
        int err = errno;
        fprintf( stderr, "accept: %s\n", strerror( err ));
        close(fd);
        fd = -1;
        return;
    }
    close(fd);
    fd = new_fd;
    connected = true;
}

PK_Message_Ext_Link_TCP :: PK_Message_Ext_Link_TCP(
    PK_Message_Ext_Handler * _handler,
    char * host, short port)
    : PK_Message_Ext_Link(_handler),
      PK_Message_Ext_Manager(_handler,this)
{
    fd = socket( AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd < 0)
    {
        int err = errno;
        fprintf( stderr, "socket: %s\n", strerror( err ));
        return;
    }

    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_port = htons( port );

    if ( ! (inet_aton( host, &sa.sin_addr )))
    {
        struct hostent * he;
        if (( he = gethostbyname( host )) == NULL )
        {
            int err = errno;
            fprintf(stderr, "host lookup of '%s': %s\n",
                    host, strerror( err ));
            close(fd);
            fd = -1;
            return;
        }
        memcpy( &sa.sin_addr.s_addr, he->h_addr, he->h_length );
    }

    if (connect( fd, (struct sockaddr *) &sa, sizeof( sa )) < 0)
    {
        int err = errno;
        fprintf( stderr, "connect: %s\n", strerror( err ));
        close(fd);
        fd = -1;
        return;
    }

    connected = true;
}

PK_Message_Ext_Link_TCP :: ~PK_Message_Ext_Link_TCP(void)
{
    if (fd > 0)
        close(fd);
}

//virtual
bool
PK_Message_Ext_Link_TCP :: write( uint8_t * buf, int buflen )
{
    uint8_t * ptr = buf;
    int cc;

    if (fd < 0)
        return false;

    while (buflen > 0)
    {
        cc = ::write(fd, ptr, buflen);
        if (cc <= 0)
        {
            connected = false;
            return false;
        }
        ptr += cc;
        buflen -= cc;
    }

    return true;
}

//virtual
int
PK_Message_Ext_Link_TCP :: read ( uint8_t * buf, int buflen, int ticks )
{
    int cc;
    fd_set rfds;
    struct timeval tv, *tvp;

    if (fd < 0)
        return false;

    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);

    if (ticks >= 0)
    {
        tv.tv_sec  =  ticks / PK_Thread::tps();
        ticks -= tv.tv_sec;
        tv.tv_usec =  ticks * (1000000 / PK_Thread::tps());
        tvp = &tv;
    }
    else
        tvp = NULL;

    cc = select(fd+1, &rfds, NULL, NULL, tvp);
    if (cc <= 0)
        return 0;

    cc = ::read(fd, buf, buflen);
    if (cc <= 0)
    {
        connected = false;
        return -1;
    }

    return cc;
}
