
#include "threads.H"
#include "AcceptorThread.H"
#include "ConfigThread.H"
#include "DataThread.H"

#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>

DataThread :: DataThread( int _fd,
                          int _connfromhost, int _connfromport,
                          int _remotehost, int _remoteport )
    : Thread( "Data", myprio, mystack )
{
    fds[0] = _fd;
    remotehost = _remotehost;
    remoteport = _remoteport;
    connfromhost = _connfromhost;
    connfromport = _connfromport;

    resume( tid );
}

DataThread :: ~DataThread( void )
{
#if VERBOSE
    printf( "closing fds %d, %d between %08x(%d) and %08x(%d)\n",
            fds[0], fds[1],
            remotehost, remoteport,
            connfromhost, connfromport );
#endif

    if ( fds[0] != -1 )
        close( fds[0] );
    if ( fds[1] != -1 )
        close( fds[1] );
}

void
DataThread :: entry( void )
{
#if VERBOSE
    printf( "attempting to establish %08x(%d) to %08x(%d)\n",
            remotehost, remoteport,
            connfromhost, connfromport );
#endif

    struct sockaddr_in sa;

    sa.sin_family = AF_INET;
    sa.sin_port = htons( remoteport );
    sa.sin_addr.s_addr = htonl( remotehost );

    fds[1] = socket( AF_INET, SOCK_STREAM, 0 );
    if ( fds[1] < 0 )
    {
        printf( "socket : %s\n", strerror( errno ));
        return;
    }

    if ( connect( fds[1], (struct sockaddr *)&sa, sizeof( sa )) < 0 )
    {
        char errbuf[80];
        sprintf( errbuf, "connect : %s\n", strerror( errno ));
        printf( errbuf );
        write( fds[0], errbuf, strlen( errbuf ));
        return;
    }

#if VERBOSE
    printf( "connected fds %d,%d\n", fds[0], fds[1] );
#endif

#if LOG
    {
        char fn[80];
        int r = random();
        sprintf( fn, "/home/flipk/conn.%d.0", r );
        logfds[0] = open( fn, O_WRONLY | O_CREAT, 0666 );
        sprintf( fn, "/home/flipk/conn.%d.1", r );
        logfds[1] = open( fn, O_WRONLY | O_CREAT, 0666 );
    }
#else
    logfds[0] = -1;
    logfds[1] = -1;
#endif

    while ( 1 )
    {
        int cc;
        int ofd;

    again:
        cc = select( 2, fds, 0, (int*)0, 1, &ofd, WAIT_FOREVER );
        if ( cc == 0 )
        {
            printf( "select returned zero!\n" );
            goto again;
        }
        if ( cc < 0 )
        {
            printf( "select : %s\n", strerror( errno ));
            if ( errno == EMFILE )
            {
                printf( "killing connection due to out-of-filedescriptor "
                        "condition\n" );
                return;
            }
            goto again;
        }

        int read_fd, write_fd;

        if ( ofd == fds[0] )
        {
            read_fd = fds[0];
            write_fd = fds[1];
        }
        else
        {
            read_fd = fds[1];
            write_fd = fds[0];
        }

        char buf[ 8192 ];
        cc = read( read_fd, buf, 8192 );
        if ( cc < 0 )
        {
            printf( "read %d : %s\n", read_fd, strerror( errno ));
            return;
        }
        if ( cc == 0 )
        {
#if VERBOSE
            printf( "read %d : connection closed\n", read_fd );
#endif
            return;
        }

        int cc2;
        cc2 = write( write_fd, buf, cc );
        if ( cc2 != cc )
        {
            printf( "write %d : %s\n", write_fd, strerror( errno ));
            return;
        }

//        printf( "%d bytes (%d -> %d)\n", cc2, read_fd, write_fd );
        if ( write_fd == fds[0] && logfds[0] != -1 )
            write( logfds[0], buf, cc );
        else if ( write_fd == fds[1] && logfds[1] != -1 )
            write( logfds[1], buf, cc );
    }
}
