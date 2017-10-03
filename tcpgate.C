
/*
 g++ tcpgate.C -Idll2 -Ithreads/h -o tgl -Dtcpgate_main=main
*/

// a simple tcpgate that just uses a fancy select construct.
// no threads, just good old-fashioned unix ingenuity.

// syntax
//   tcpgate <port> [-c | -u] <host> <port>   [ more port/comp/host/ports]

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/uio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>

#include "tcpgate.H"

static FILE * logfd;

#define VERBOSE 0

extern "C" int tcpgate_main( int argc, char ** argv );
extern "C" int pipe_main( int argc, char ** argv );

static FDMAP_LIST * list;

FDMAP_DATA :: FDMAP_DATA( int _fd, bool connecting )
    : buf( BUFSIZE )
{
    fd              = _fd;
    can_read        = connecting;
    want_write      = connecting;
    waitfor_connect = connecting;
    want_close      = false;
}

#if defined(SUNOS) || defined(SOLARIS) || defined(CYGWIN)
#define setsockoptcast char*
#else
#define setsockoptcast void*
#endif

void
FDMAP_LISTEN :: handle_select_r( void )
{
    int ear, nfd;
    struct sockaddr_in sa;
    socklen_t salen = sizeof( sa );

    ear = accept( fd, (struct sockaddr *)&sa, &salen );

    if ( ear < 0 )
    {
        fprintf( stderr, "accept error on fd %d: %s\n",
                 fd, strerror( errno ));
        exit(1);
    }

    nfd = socket( AF_INET, SOCK_STREAM, 0 );

    if ( nfd < 0 )
    {
        fprintf( stderr, "alllocation of socket failed:  %s\n",
                 strerror( errno ));
        close( ear );
        return;
    }

    sa.sin_family = AF_INET;
    sa.sin_port = htons( remote_port );
    sa.sin_addr = remote_host;

    fcntl( nfd, F_SETFL, fcntl( nfd, F_GETFL, 0 ) | O_NONBLOCK );
    fcntl( ear, F_SETFL, fcntl( ear, F_GETFL, 0 ) | O_NONBLOCK );

    // perform the connect in async mode; when the call
    // is made, it is correct for it to fail with EINPROGRESS.
    // the connect continues in the background.
    // once the connection succeeds, a select-for-write will
    // complete successfully. if the connection fails, a 
    // select-for-read will complete first.

    // actually i'm not so sure about that. it might be bullshit.
    // but the getpeername() trick in select_w seems to work better.

    if ( connect( nfd, (struct sockaddr *)&sa, sizeof( sa )) < 0 )
    {
        if ( errno != EINPROGRESS )
        {
            fprintf( stderr,
                     "connect failed: %s\n", strerror( errno ));
            close( ear );
            close( nfd );
            return;
        }
    }

    FDMAP_DATA * a = new FDMAP_DATA( nfd, true  );
    FDMAP_DATA * b = new FDMAP_DATA( ear, false );

    a->set_other( b );
    b->set_other( a );

    list->add( a );
    list->add( b );
}

void
FDMAP_DATA :: handle_select_w( void )
{
    if ( waitfor_connect )
    {
        struct sockaddr_in name;
        socklen_t namelen = sizeof( name );

        if ( getpeername( fd, (struct sockaddr *)&name, &namelen ) < 0 )
            if ( errno == ENOTCONN )
            {
                printf( "fd %d not connected during write\n", fd );
                closeit();
                return;
            }

        // this means the connect call completed and has 
        // successfully established.
#if VERBOSE
        printf( "fd %d connected\n", fd );
#endif
        waitfor_connect = false;
    }

    if ( !want_close )
        other_fd->can_read = true;

    int bufsize = buf.contig_readable();
    if ( bufsize > 0 )
    {
        int cc = write( fd, buf.read_pos(), bufsize );
#if VERBOSE
        printf( "wrote %d of %d bytes\n", cc, bufsize );
#endif

        if ( cc <= 0 )
            closeit();
        else
            buf.record_read( cc );
    }

    if ( buf.empty() )
    {
        want_write = false;
        if ( want_close )
            closeit();
    }
}

void
FDMAP_DATA :: handle_select_r( void )
{
    if ( waitfor_connect )
    {
        // this means the connect call has failed and the
        // connection was not established.
        waitfor_connect = false;
#if VERBOSE
        printf( "fd %d NOT connected\n", fd );
#endif
        closeit();
        return;
    }
    other_fd->_handle_select_r();
}

void
FDMAP_DATA :: _handle_select_r( void )
{
    int bufsize = buf.contig_writeable();
    int cc = read( other_fd->fd, buf.write_pos(), bufsize );

    if ( logfd )
    {
        unsigned char * bp = (unsigned char *) buf.write_pos();
        fprintf( logfd, "data read from fd %d (%d bytes)\n", 
                 other_fd->fd, cc );
        for ( int i = 0; i < cc; i++ )
            fprintf( logfd, "%02x ", bp[i] );
        fprintf( logfd, "\n" );
        for ( int i = 0; i < cc; i++ )
        {
            unsigned char c = bp[i];
            if ( c < 0x20 || c > 0x7f )
                c = '.';
            fprintf( logfd, "%c", c );
        }
        fprintf( logfd, "\n" );
    }

    int e = cc < 0 ? errno : 0;
#if VERBOSE
    printf( "read %d of %d: %s\n",
            cc, bufsize,
            e == 0 ? "no error" : strerror( e ));
#endif

    if ( cc < 0 )
    {
        if ( e != EAGAIN )
            closeit();
    }
    else if ( cc == 0 )
    {
        other_fd->can_read = false;
        // flag that the connection is dead
        // but don't close the writer yet.
        // there could still be data in the buffer.
        // let the writer wake up and when the
        // writer empties the buffer it will 
        // do the closeit.
        want_close = true;
        want_write = true;
    }
    else
    {
        buf.record_write( cc );
        want_write = true;
    }

    if ( buf.full() )
        other_fd->can_read = false;
}

static void
manage_ports_loop( void )
{
    if ( list->get_cnt() == 0 )
    {
        fprintf( stderr, "no ports to forward!\n" );
        return;
    }

    // now start main event loop

    fd_set zrfds;
    FD_ZERO( &zrfds );

    while ( 1 )
    {
        fd_set rfds, wfds;
        int cc, max;
        FDMAP * m, * nm;

        FD_ZERO( &rfds );
        FD_ZERO( &wfds );

        max = 0;
        for ( m = list->get_head(); m; m = list->get_next(m) )
        {
            if ( m->fd != -1 )
            {
                bool inc_max = false;
                if ( m->sel_r() )
                {
                    FD_SET( m->fd, &rfds );
                    inc_max = true;
                }
                if ( m->sel_w() )
                {
                    FD_SET( m->fd, &wfds );
                    inc_max = true;
                }
                if ( inc_max )
                    if ((m->fd+1) > max )
                        max = m->fd + 1;
            }
        }

        cc = select( max, &rfds, &wfds, NULL, NULL );
        if ( cc < 0 )
        {
            fprintf( stderr, "select error: %s\n", strerror( errno ));
            return;
        }

        FDMAP_DELETE_LIST del;

        if ( cc > 0 )
        {
#if 0 /* for debug */
            for ( int i = 0; i < max; i++ )
            {
                if ( FD_ISSET(i, &rfds))
                    printf( "select %d for read\n", i );
                if ( FD_ISSET(i, &wfds))
                    printf( "select %d for write\n", i );
            }
#endif
            for ( m = list->get_head(); m; m = nm )
            {
                nm = list->get_next(m);
                if ( m->fd != -1 )
                    if ( FD_ISSET( m->fd, &wfds ))
                        m->handle_select_w();
                if ( m->fd != -1 )
                    if ( FD_ISSET( m->fd, &rfds ))
                        m->handle_select_r();
                if ( m->fd == -1 )
                    del.add( m );
            }
        }

        // delete any fds which require deleting

        for ( m = del.get_head(); m; m = nm )
        {
            nm = del.get_next( m );
            list->remove( m );
            del.remove( m );
            delete m;
        }
    }
    // NOTREACHED
}

int
tcpgate_main( int argc, char ** argv )
{
    logfd = NULL;
    list = new FDMAP_LIST;

    argc -= 1;
    argv += 1;

    while ( argc != 0 )
    {
        struct sockaddr_in sa;
        struct in_addr remote_host;
        struct hostent *he;
        int fd, siz, port, remote_port;
        FDMAP_LISTEN * l;
        int salen;
        char * host;

        if ( argc > 1 && strncmp( argv[0], "-l", 2 ) == 0 )
        {
            logfd = fopen( argv[0]+2, "w" );
            if ( logfd == NULL )
            {
                fprintf( stderr, "unable to open log!\n" );
                delete list;
                return 1;
            }
            setvbuf( logfd, NULL, _IOLBF, 0 );
            argv++;
            argc--;
        }

        if ( argc < 3 )
        {
            fprintf( stderr, "arg parsing problem\n" );
            delete list;
            return 1;
        }

        port        = atoi( argv[0] );
        host        =       argv[1];
        remote_port = atoi( argv[2] );

        argc -= 3;
        argv += 3;

        if ( inet_aton( host, &sa.sin_addr ))
        {
            remote_host = sa.sin_addr;
        }
        else
        {
            if (( he = gethostbyname( host )) == NULL )
            {
                fprintf( stderr,
                         "could not resolve hostname '%s'\n", argv[1] );
                delete list;
                return 1;
            }
            memcpy( &remote_host, he->h_addr, 4 );
        }

        fd = socket( AF_INET, SOCK_STREAM, 0 );

        if ( fd < 0 )
        {
            fprintf( stderr, "socket failed: %s\n", strerror( errno ));
            delete list;
            return 1;
        }

        siz = 1;
        (void)setsockopt( fd, SOL_SOCKET, SO_REUSEADDR,
                          (setsockoptcast) &fd, sizeof( fd ));

        sa.sin_family = AF_INET;
        sa.sin_port = htons(port);
        sa.sin_addr.s_addr = INADDR_ANY;

        if ( bind( fd, (struct sockaddr *)&sa, sizeof( sa )) < 0 )
        {
            fprintf( stderr,
                     "bind on port %d failed: %s\n", port,
                     strerror( errno ));
            delete list;
            return 1;
        }

        listen( fd, 10 );

        l = new FDMAP_LISTEN( fd, port, remote_port, remote_host );
        list->add( l );
    }

    manage_ports_loop();

    delete list;
    return 0;
}
