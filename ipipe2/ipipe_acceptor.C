
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
        fprintf( stderr, "socket: %s\n", strerror( errno ));
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
        fprintf( stderr, "bind: %s\n", strerror( errno ));
        exit( 1 );
    }
    listen( fd, 1 );
}

//virtual
ipipe_acceptor :: ~ipipe_acceptor( void )
{
    delete connection_factory;
    close( fd );
}

//virtual
bool
ipipe_acceptor :: select_for_read( fd_mgr * mgr )
{
    // always
    return true;
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
        fprintf( stderr, "accept: %s\n", strerror( errno ));
        return OK;
    }

    if ( connection_factory->new_conn( mgr, new_fd ) ==
         ipipe_new_connection::CONN_DONE )
    {
        return DEL;
    }

    return OK;
}

//virtual
bool
ipipe_acceptor :: select_for_write( fd_mgr * mgr )
{
    // never
    return false;
}

//virtual
fd_interface :: rw_response
ipipe_acceptor :: write( fd_mgr * mgr )
{
    //error
    return DEL;
}
