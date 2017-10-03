
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

    make_nonblocking();

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
        fprintf( stderr, "connect: Unable to connect\n" );
        return DEL;
    }

    (void) connection_factory->new_conn( mgr, fd );
    return DEL;
}
