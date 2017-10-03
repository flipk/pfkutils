
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
ipipe_connector :: read ( fd_mgr * )
{
    int cc;
    char buf[ 1 ];
    
    cc = ::read( fd, buf, 1 );
    if ( cc >= 0 )
    {
        fprintf( stderr, "connector read returned %d??\n", cc );
        exit( 1 );
    }
    fprintf( stderr, "connect: %s\n", strerror( errno ));
    exit( 1 );
}

//virtual
bool
ipipe_connector :: write( fd_mgr * mgr )
{
    connection_factory->new_conn( mgr, fd );
    do_close = true;
    return false;
}

//virtual
bool
ipipe_connector :: select_for_read( fd_mgr * )
{
    if ( !do_close )
        return true;
    return false;
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
bool
ipipe_connector :: over_write_threshold( void )
{
    // not applicable
    return false;
}

//virtual
bool
ipipe_connector :: write_to_fd( char * buf, int len )
{
    // not applicable
    return false;
}
