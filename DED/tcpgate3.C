
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

#include "threads.H"

// symbol conflict with tcpgate.C which defines all these
// methods too; rename them.

#define FDMAP GATE3_FDMAP
#define FDMAP_DATA GATE3_FDMAP_DATA
#define FDMAP_LISTEN GATE3_FDMAP_LISTEN

#include "tcpgate.H"

#define VERBOSE 0

static FDMAP_LIST        * list;
static FDMAP_DELETE_LIST * dellist;
static FDMAP_LIST        * newlist;

class TcpGate3Stats;
static TcpGate3Stats * gatestats;

class TcpGate3Stats  : public Thread {
    void entry( void );
    int bytes;
public:
    TcpGate3Stats( void )
        : Thread( "stats", 5, 10000 )
        {
            bytes = 0;
            gatestats = this;
            resume( tid );
        }
    ~TcpGate3Stats( void ) { }
    void register_bytes( int v ) { bytes += v; }
};

void
TcpGate3Stats :: entry( void )
{
    int i = 0;
    while ( 1 )
    {
        printf( "bytes: %d\n", bytes );
        sleep( tps() );
        if ( ++i == 10 )
        {
            i = 0;
            th->printinfo();
        }
    }
}

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
#define socklen_t int
#define setsockoptcast char*
#else
#define setsockoptcast void*
#endif

void
FDMAP_LISTEN :: handle_select_r( void )
{
    int ear, nfd;
    struct sockaddr_in sa;
    int salen = sizeof( sa );

    ear = accept( fd, (struct sockaddr *)&sa, (socklen_t*)&salen );

    if ( ear < 0 )
    {
        printf( "accept error on fd %d: %s\n", fd, strerror( errno ));
        exit(1);
    }

    nfd = socket( AF_INET, SOCK_STREAM, 0 );

    if ( nfd < 0 )
    {
        printf( "alllocation of socket failed:  %s\n", strerror( errno ));
        close( ear );
        return;
    }

    sa.sin_family = AF_INET;
    sa.sin_port = htons( remote_port );
    sa.sin_addr = remote_host;

    th->register_fd( nfd );
    th->register_fd( ear );

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
            printf( "connect failed: %s\n", strerror( errno ));
            close( ear );
            close( nfd );
            return;
        }
    }

    FDMAP_DATA * a = new FDMAP_DATA( nfd, true  );
    FDMAP_DATA * b = new FDMAP_DATA( ear, false );

    a->set_other( b );
    b->set_other( a );

    newlist->add( a );
    newlist->add( b );
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
                dellist->add( this );
                dellist->add( other_fd );
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
        int cc = th->write( fd, buf.read_pos(), bufsize );
#if VERBOSE
        printf( "wrote %d of %d bytes\n", cc, bufsize );
#endif

        if ( cc <= 0 )
        {
            closeit();
            dellist->add( this );
            dellist->add( other_fd );
        }
        else
        {
            buf.record_read( cc );
            gatestats->register_bytes( cc );
        }
    }

    if ( buf.empty() )
    {
        want_write = false;
        if ( want_close )
        {
            closeit();
            dellist->add( this );
            dellist->add( other_fd );
        }
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
        dellist->add( this );
        dellist->add( other_fd );
        return;
    }
    other_fd->_handle_select_r();
}

void
FDMAP_DATA :: _handle_select_r( void )
{
    int bufsize = buf.contig_writeable();
    int cc = th->read( other_fd->fd, buf.write_pos(), bufsize );
    int e = cc < 0 ? errno : 0;
#if VERBOSE
    printf( "read %d of %d: %s\n",
            cc, bufsize,
            e == 0 ? "no error" : strerror( e ));
#endif

    if ( cc < 0 )
    {
        if ( e != EAGAIN )
        {
            closeit();
            dellist->add( this );
            dellist->add( other_fd );
        }
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

class TcpGate3Thread : public Thread {
    void entry( void );
    void maybereg( FDMAP * fdm );
    int mq;
    int argc;
    char ** argv;
public:
    TcpGate3Thread( int _argc, char ** _argv )
        : Thread( "main", 10, 32768 )
        {
            argc = _argc; argv = _argv; resume( tid );
        }
    ~TcpGate3Thread( void ) { }
};

void
TcpGate3Thread :: maybereg( FDMAP * fdm )
{
    int forval;
    if ( fdm->fd != -1 )
    {
        forval = FOR_NOTHING;
        if ( fdm->sel_r() ) forval += FOR_READ;
        if ( fdm->sel_w() ) forval += FOR_WRITE;
        if ( forval != FOR_NOTHING )
        {
            register_fd_mq( fdm->fd, (void*)fdm, (fd_mq_t)forval, mq );
#if VERBOSE
            printf( "fd %d registered for %d\n", fdm->fd, forval );
#endif
        }
    }
    fdm = fdm->other_fd;
    if ( fdm && fdm->fd != -1 )
    {
        forval = FOR_NOTHING;
        if ( fdm->sel_r() ) forval += FOR_READ;
        if ( fdm->sel_w() ) forval += FOR_WRITE;
        if ( forval != FOR_NOTHING )
        {
            register_fd_mq( fdm->fd, (void*)fdm, (fd_mq_t)forval, mq );
#if VERBOSE
            printf( "fd %d registered for %d\n", fdm->fd, forval );
#endif
        }
    }
}

void
TcpGate3Thread :: entry( void )
{
    list    = new FDMAP_LIST;
    dellist = new FDMAP_DELETE_LIST;
    newlist = new FDMAP_LIST;

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

        if ( argc < 3 )
        {
            printf( "arg parsing problem\n" );
            delete list;
        }

        port        = atoi( argv[0] );
        host        =       argv[1]  ;
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
                printf( "could not resolve hostname '%s'\n", argv[1] );
                delete list;
            }
            memcpy( &remote_host, he->h_addr, 4 );
        }

        fd = socket( AF_INET, SOCK_STREAM, 0 );
        if ( fd < 0 )
        {
            printf( "socket failed: %s\n", strerror( errno ));
            delete list;
        }

        siz = 1;
        (void)setsockopt( fd, SOL_SOCKET, SO_REUSEADDR,
                          (setsockoptcast) &fd, sizeof( fd ));

        sa.sin_family = AF_INET;
        sa.sin_port = htons(port);
        sa.sin_addr.s_addr = INADDR_ANY;

        if ( bind( fd, (struct sockaddr *)&sa, sizeof( sa )) < 0 )
        {
            printf( "bind on port %d failed: %s\n", port, strerror( errno ));
            delete list;
        }

        listen( fd, 10 );

        l = new FDMAP_LISTEN( fd, port, remote_port, remote_host );
        list->add( l );
    }

    if ( list->get_cnt() == 0 )
    {
        printf( "no ports to forward!\n" );
        return;
    }

    // now start main event loop

    if ( register_mq( mq, "fdactive" ) == false )
    {
        printf( "could not register mq\n" );
        return;
    }

    FDMAP * fdm, * fdnm;

    for ( fdm = list->get_head(); fdm; fdm = list->get_next(fdm) )
        maybereg( fdm );

    while ( 1 )
    {
        union {
            FdActiveMessage * fam;
            Message * m;
        } m;
        int mqout;

        m.m = recv( 1, &mq, &mqout, WAIT_FOREVER );

        if ( m.m->type.get() != FdActiveMessage::TYPE )
        {
            printf( "unknown type %#x received\n", m.m->type.get() );
            delete m.m;
            continue;
        }

        fdm = (FDMAP*) m.fam->arg;
        fd_mq_t activity = (fd_mq_t) m.fam->activity;
        delete m.m;

#if VERBOSE
        printf( "fd %d selected for %s\n",
                fdm->fd,
                (activity == FOR_READ) ? "read" : "write" );
#endif

        if ( activity == FOR_READ )
            fdm->handle_select_r();
        else
            fdm->handle_select_w();

        maybereg( fdm );

        for ( fdm = dellist->get_head(); fdm; fdm = fdnm )
        {
            fdnm = dellist->get_next( fdm );
            list->remove( fdm );
            dellist->remove( fdm );
            delete fdm;
        }

        for ( fdm = newlist->get_head(); fdm; fdm = fdnm )
        {
            fdnm = newlist->get_next( fdm );
            newlist->remove( fdm );
            list->add( fdm );
            maybereg( fdm );
        }
    }

    delete list;
    delete dellist;
    delete newlist;
}

extern "C" int tcpgate3_main( int argc, char ** argv );

int
tcpgate3_main( int argc, char ** argv )
{
    ThreadParams p;
    p.my_eid = 1;
    Threads th( &p );
    new TcpGate3Thread( argc, argv );
    new TcpGate3Stats;
    th.loop();
    return 0;
}
