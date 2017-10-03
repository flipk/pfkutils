
/*
    This file is part of the "pkutils" tools written by Phil Knaack
    (pknaack1@netscape.net).
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

/*
 * if itsfssvr bus faults, any command accessing the /i filesystem will
 * hang, indefinitely.  in order to get around this problem, this program
 * provides a way out.  after itsfssvr exits, start this program to bind
 * the udp port and provide NFS RPC replies to the NFS requests.  it does
 * not support any NFS RPC, and replies to all queries with error packets,
 * however this is enough to allow you to unmount /i and continue on.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <signal.h>
#include <stdarg.h>
#include <string.h>

#include "config.h"
#include "xdr.h"
#include "rpc.h"

extern "C" void *
malloc_record( char * file, int line, size_t size )
{
    void * ret = (void*) malloc( size );
    return ret;
}

int
main()
{
    int fd;
    struct sockaddr_in sa;

    fd = socket( AF_INET, SOCK_DGRAM, 0 );
    if ( fd < 0 )
    {
        printf( "socket failed: %d\n", errno );
        return 1;
    }

    int flag = 1;
    if ( setsockopt( fd, SOL_SOCKET, SO_REUSEADDR,
                     (char*)&flag, sizeof( flag )) < 0 )
    {
        printf( "setsockopt failed: %d\n", errno );
        return 1;
    }

    sa.sin_family = AF_INET;
    sa.sin_port = htons( SERVER_PORT );
    sa.sin_addr.s_addr = INADDR_ANY;

    if ( bind( fd, (struct sockaddr *)&sa, sizeof( sa )) < 0 )
    {
        printf( "bind failed: %d\n", errno );
        return 1;
    }

    uchar         buf[ 9000 ];
    int           cc;
    rpccallmsg    call;
    rpcreplymsg   reply;
    XDR           xdr;

    while ( 1 )
    {
        xdr.encode_decode  = XDR_DECODE;
        xdr.data           = buf;
        xdr.position       = 0;
        xdr.bytes_left     = sizeof( buf );

        socklen_t salen = sizeof( sa );
        cc = recvfrom( fd, buf, sizeof( buf ),
                       0, // flags
                       (struct sockaddr *)&sa, &salen );
        printf( "got packet of size %d\n", cc );

        memset( &call,  0, sizeof( call  ));
        memset( &reply, 0, sizeof( reply ));

        if ( myxdr_rpccallmsg( &xdr, &call ) == TRUE )
        {
            reply.xid         = call.xid;
            reply.dir         = RPC_REPLY_MSG;
            reply.stat        = RPC_MSG_ACCEPTED;
            reply.verf_flavor = 0;
            reply.verf_length = 0;
            reply.verf_base   = NULL;
            reply.verf_stat   = RPC_AUTH_OK;

            xdr.encode_decode = XDR_ENCODE;
            xdr.data          = buf;
            xdr.position      = 0;
            xdr.bytes_left    = sizeof( buf );

            myxdr_rpcreplymsg( &xdr, &reply );

            printf( "reply is %d bytes long\n", xdr.position );

            sendto( fd, buf, xdr.position,
                    0, // flags
                    (struct sockaddr *)&sa, sizeof( sa ));

            xdr.encode_decode = XDR_FREE;
            myxdr_rpccallmsg( &xdr, &call );
        }
        else
        {
            printf( "error decoding rpc call\n" );
        }
    }
}
