
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
        fprintf( stderr, "socket: %s\n", strerror( errno ));
        exit( 1 );
    }

    fcntl( fd, F_SETFL,
           fcntl( fd, F_GETFL, 0 ) | O_NONBLOCK );

    int cc = connect( fd, (struct sockaddr *)sa, sizeof( *sa ));

    if ( cc == 0 )
    {
        fprintf( stderr, "connect SUCCEEDED????\n" );
        exit( 1 );
    }

    if ( cc < 0  &&  errno != EINPROGRESS )
    {
        fprintf( stderr, "connect: %s\n", strerror( errno ));
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
bool
ipipe_connector :: select_for_read( fd_mgr * )
{
    // never
    return false;
}

//virtual
fd_interface :: rw_response
ipipe_connector :: read ( fd_mgr * )
{
    return DEL;
}

//virtual
bool
ipipe_connector :: select_for_write( fd_mgr * )
{
    if ( !do_close )
        return true;
    return false;
}

//virtual
fd_interface :: rw_response
ipipe_connector :: write( fd_mgr * mgr )
{
    int cc;
    char buf[1];
    // BIG XXX BIG XXX    does this work on other OS's ???
    cc = ::read( fd, buf, 0 );
    if ( cc < 0 )
        fprintf( stderr, "connect: %s\n", strerror( errno ));
    else
        (void) connection_factory->new_conn( mgr, fd );
    return DEL;
}
