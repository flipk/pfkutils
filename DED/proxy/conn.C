
#include "conn.H"
#include "proxy.H"

connThread :: connThread( void )
    : Thread( "conn", myprio, mystack )
{
    fd = -1;
    death_requested = false;
    resume( tid );
}

connThread :: ~connThread( void )
{
    if ( fd != -1 )
        close( fd );
}

void
connThread :: entry( void )
{
    fd = socket( AF_INET, SOCK_STREAM, 0 );
    if ( fd < 0 )
    {
        printf( "socket: %s\n", strerror( errno ));
        return;
    }

    int flag = 1;

    setsockopt( fd, SOL_SOCKET, SO_REUSEADDR,
                &flag, sizeof( flag ));

    sa.sin_family = AF_INET;
    sa.sin_port = htons( myport );
    sa.sin_addr.s_addr = INADDR_ANY;

    if ( bind( fd, (struct sockaddr *)&sa, sizeof( sa )) < 0 )
    {
        printf( "bind: %s\n", strerror( errno ));
        return;
    }

    listen( fd, 10 );

    while ( 1 )
    {
        int ofds;
        int cc;

        cc = select( 1, &fd, 0, 0, 1, &ofds, WAIT_FOREVER );
        if ( death_requested )
            return;
        if ( cc == 0 )
        {
            printf( "select returned zero\n" );
            continue;
        }
        if ( cc < 0 )
        {
            printf( "select : %s\n", strerror( errno ));
            continue;
        }

        socklen_t sizeaddr = sizeof( sa );
        int nfd = accept( fd, (struct sockaddr *)&sa, &sizeaddr );
        if ( nfd < 0 )
        {
            printf( "accept : %s\n", strerror( errno ));
            continue;
        }

        (void) new proxyThread( nfd );
    }
}
