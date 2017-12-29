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

#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "ipipe_acceptor.h"

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
