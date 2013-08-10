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
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "ipipe_connector.H"

ipipe_connector :: ipipe_connector( struct sockaddr_in * sa,
                                    ipipe_new_connection * inc )
{
    connection_factory = inc;

    fd = socket( AF_INET, SOCK_STREAM, 0 );
    if ( fd < 0 )
    {
        fprintf( stderr, "\nsocket: %s\n", strerror( errno ));
        exit( 1 );
    }

    make_nonblocking();

    int cc = connect( fd, (struct sockaddr *)sa, sizeof( *sa ));

    if ( cc < 0  &&  errno != EINPROGRESS )
    {
        fprintf( stderr, "\nconnect: %s\n", strerror( errno ));
        exit( 1 );
    }
}

//virtual
ipipe_connector :: ~ipipe_connector( void )
{
    delete connection_factory;
    // no close! the fd was passed on
}

//virtual
void
ipipe_connector :: select_rw ( fd_mgr *, bool * rd, bool * wr )
{
    *rd = false;

    if ( !do_close )
        *wr = true;
    else
        *wr = false;
}

//virtual
fd_interface :: rw_response
ipipe_connector :: write( fd_mgr * mgr )
{
    int cc;
    struct sockaddr_in sa;
    socklen_t salen = sizeof(sa);

    cc = getpeername( fd, (struct sockaddr *)&sa, &salen );

    if ( cc < 0 )
    {
        fprintf( stderr, "\nconnect: Unable to connect\n" );
        return DEL;
    }

    (void) connection_factory->new_conn( mgr, fd );
    return DEL;
}
