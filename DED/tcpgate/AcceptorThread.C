
#include "threads.H"
#include "AcceptorThread.H"
#include "ConfigThread.H"
#include "DataThread.H"

#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

#define socklen_type socklen_t
#define setsockoptcast char *

AcceptorThread :: AcceptorThread( int _port,
                                  int _remotehost,
                                  int _remoteport )
    : Thread( "Acceptor", myprio, mystack )
{
    port = _port;
    remotehost = _remotehost;
    remoteport = _remoteport;
    fd = -1;

    resume( tid );
}

AcceptorThread :: ~AcceptorThread( void )
{
    if ( fd > 0 )
        close( fd );
}

void
AcceptorThread :: entry( void )
{
#if VERBOSE
    printf( "starting Acceptor for port %d, "
            "gateway to host %08x port %d\n",
            port, remotehost, remoteport );
#endif

    fd = socket( AF_INET, SOCK_STREAM, 0 );
    if ( fd < 0 )
    {
        printf( "socket: %s\n", strerror( errno ));
        return;
    }

    int flag = 1;

    setsockopt( fd, SOL_SOCKET, SO_REUSEADDR,
                (setsockoptcast)&flag, sizeof( flag ));

    struct sockaddr_in sa;

    sa.sin_family = AF_INET;
    sa.sin_port = htons( port );
    sa.sin_addr.s_addr = INADDR_ANY;

    if ( bind( fd, (struct sockaddr *)&sa, sizeof( sa )) < 0 )
    {
        printf( "bind: %s\n", strerror( errno ));
        return;
    }

    listen( fd, 1 );

    while ( 1 )
    {
        int ofds;
        int cc;

        cc = select( 1, &fd, 0, 0, 1, &ofds, WAIT_FOREVER );
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

        socklen_type sizeaddr = sizeof( sa );
        int nfd = accept( fd, (struct sockaddr *)&sa, &sizeaddr );
        if ( nfd < 0 )
        {
            printf( "accept : %s\n", strerror( errno ));
            continue;
        }

        if ( remotehost == CONFIG_HOST )
        {
            (void) new ConfigThread( nfd );
        }
        else if ( remotehost == STATUS_HOST )
        {
            FILE * f = fdopen( nfd, "w" );
            th->printinfo2( f );
            fclose( f );
        }
        else
        {
            try {
                (void) new DataThread( nfd,
                                       ntohl( sa.sin_addr.s_addr ),
                                       ntohs( sa.sin_port ),
                                       remotehost, remoteport );
            }
            catch (...)
            {
                printf( "unable to create data thread, "
                        "ditching connection on fd %d\n", nfd );
                close( nfd );
            }
        }
    }
}
