
#include "main.H"
#include "acceptor.H"
#include "client.H"

Acceptor :: Acceptor( int _port )
    : Thread( "Acceptor", MY_PRIO )
{
    port = _port;
    printf( "Acceptor thread started, port is %d\n", port );
    resume( tid );
}

Acceptor :: ~Acceptor( void )
{
    printf( "Acceptor: exiting\n" );
    global_exit = true;
    if ( fd != -1 )
        close( fd );
}

void
Acceptor :: entry( void )
{
    struct sockaddr_in sa;
    int fd_errors = 0;

    fd = -1;

    fd = socket( AF_INET, SOCK_STREAM, 0 );
    if ( fd < 0 )
    {
        printf( "Acceptor: socket failed! (%s)\n",
                strerror( errno ));
        return;
    }

    int soval = 1;
    setsockopt( fd, SOL_SOCKET, SO_REUSEADDR,
                &soval, sizeof( soval ));

    sa.sin_family      = AF_INET;
    sa.sin_port    = htons( port );
    sa.sin_addr.s_addr = INADDR_ANY;

    if ( bind( fd, (struct sockaddr *)&sa, sizeof( sa )) < 0 )
    {
        printf( "Acceptor: bind failed! (%s)\n",
                strerror( errno ));
        return;
    }

    listen( fd, 1 );

    printf( "Acceptor: listening on fd %d\n", fd );

    while ( global_exit == false )
    {
        unsigned int sz;
        int out_fd;
        int cc;

        cc = select( 1, &fd, 0, NULL, 1, &out_fd, 10 );
        if ( cc != 1 )
        {
            if ( fd_errors > 0 )
                fd_errors--;
            continue;
        }

        sz = sizeof( sa );
    again:
        out_fd = accept( fd, (struct sockaddr *)&sa, &sz );
        if ( out_fd < 0  && errno == EINTR )
        {
            if ( ++fd_errors >= 15 )
            {
                printf( "Acceptor: accept interrupted "
                        "for the last time\n" );
                return;
            }
            printf( "Acceptor: accept interrupted\n" );
            goto again;
        }

        if ( out_fd < 0 )
        {
            printf( "Acceptor: accept failed! (%s)\n",
                    strerror( errno ));
            if ( ++fd_errors == 10 )
            {
                printf( "Acceptor: giving up on accept\n" );
                return;
            }
            continue;
        }

        (void) new Client( out_fd, sa.sin_addr.s_addr );
    }
}
