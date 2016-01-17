/**
 * \file pk_messages_ext_link_tcp.cc
 * \brief implementation of external messaging link over TCP
 * \author Phillip F Knaack <pfk@pfk.org>

    This file is part of the "pfkutils" tools written by Phil Knaack
    (pfk@pfk.org).
    Copyright (C) 2008  Phillip F Knaack

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
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
    fd = socket( AF_INET, SOCK_STREAM, 0);
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
    int new_fd = accept( fd, (struct sockaddr *) &sa, &salen );
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
    fd = socket( AF_INET, SOCK_STREAM, 0);
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
