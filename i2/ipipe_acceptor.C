/*
    This file is part of the "pfkutils" tools written by Phil Knaack
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

#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "ipipe_acceptor.H"

ipipe_acceptor :: ipipe_acceptor( short port, ipipe_new_connection * _factory )
{
    connection_factory = _factory;
    fd = socket( AF_INET, SOCK_STREAM, 0 );
    if ( fd < 0 )
    {
        fprintf( stderr, "\nsocket: %s\n", strerror( errno ));
        exit( 1 );
    }
    int v = 1;
    setsockopt( fd, SOL_SOCKET, SO_REUSEADDR, (void*) &v, sizeof( v ));
    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_port = htons( port );
    sa.sin_addr.s_addr = INADDR_ANY;
    if ( bind( fd, (struct sockaddr *)&sa, sizeof( sa )) < 0 )
    {
        fprintf( stderr, "\nbind: %s\n", strerror( errno ));
        exit( 1 );
    }
    listen( fd, 1 );
    make_nonblocking();
}

//virtual
ipipe_acceptor :: ~ipipe_acceptor( void )
{
    delete connection_factory;
    close( fd );
}

//virtual
void
ipipe_acceptor :: select_rw ( fd_mgr * mgr, bool * rd, bool * wr )
{
    *rd = true;
    *wr = false;
}

//virtual
fd_interface :: rw_response
ipipe_acceptor :: read ( fd_mgr * mgr )
{ 
    struct sockaddr_in sa;
    socklen_t  salen;

    salen = sizeof( sa );
    int new_fd = accept( fd, (struct sockaddr *)&sa, &salen );

    if ( new_fd < 0 )
    {
        if ( errno != EAGAIN )
            fprintf( stderr, "\naccept: %s\n", strerror( errno ));
        return OK;
    }

    if ( connection_factory->new_conn( mgr, new_fd ) ==
         ipipe_new_connection::CONN_DONE )
    {
        return DEL;
    }

    return OK;
}
