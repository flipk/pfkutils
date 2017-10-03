#if 0
g++ -Ithreads/h -Idll2 tcpgate3.C threads/libthreads.a -g3 -o gate3
exit 0
#endif

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
#include "dll2.H"

#if defined(SUNOS) || defined(SOLARIS) || defined(CYGWIN)
#define socklen_t int
#define setsockoptcast char*
#else
#define setsockoptcast void*
#endif

struct conn {
    LListLinks<conn> links[1];
    bool listen;
    int fds[2];
    struct in_addr addr;
    int port;
    int bytes;

    conn( int fd, struct in_addr _addr, int _port ) {
        listen = true;
        fds[0] = fd; fds[1] = -1;
        addr = _addr;
        port = _port;
        bytes = 0;
    }
    conn( int fd1, int fd2 ) {
        listen = false;
        fds[0] = fd1;
        fds[1] = fd2;
        bytes = 0;
    }
    ~conn(void) {
        close( fds[0] );
        if ( !listen )
            fds[1];
    }
};

class MainThread : public Thread {
    void entry( void );
    void handle_fd( int fd, conn * c );
    int mq;
    int argc;
    char ** argv;
    int bytes;
    LList<conn,0>  conns;
public:
    MainThread( int _argc, char ** _argv )
        : Thread( "Main", 20, 18000 )
        {
            argc = _argc;
            argv = _argv;
            resume( tid );
        }
    ~MainThread(void) {
        conn * c, * nc;
        for ( c = conns.get_head(); c; c = nc )
        {
            nc = conns.get_next(c);
            conns.remove(c);
            delete c;
        }
    }
};

int
main( int argc, char ** argv )
{
    ThreadParams p;
    p.my_eid = 1;
    p.max_threads = 512;
    p.max_fds = 1024;

    Threads th( &p );
    new MainThread( argc, argv );
    th.loop();
    return 0;
}

class MainTimer : public Message {
public:
    static const unsigned int TYPE = 0x23c64c1f;
    MainTimer( void ) :
        Message( sizeof( MainTimer ), TYPE ) { }
};

void
MainThread :: entry( void )
{
    conn * c;

    if ( register_mq( mq, "mainthread" ) == false )
    {
        printf( "unable to register mq\n" );
        return;
    }

    argc -= 1;
    argv += 1;
    bytes = 0;

    while ( argc > 0 )
    {
        struct sockaddr_in sa;
        socklen_t salen;
        struct in_addr remote_host;
        struct hostent * he;
        int fd, siz, port, remote_port;
        char * host;

        if ( argc < 3 )
        {
            printf( "arg parsing problem\n" );
            return;
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
                printf( "unknown hostname '%s'\n", host );
                return;
            }
            memcpy( &remote_host, he->h_addr, 4 );
        }

        fd = socket( AF_INET, SOCK_STREAM, 0 );
        if ( fd < 0 )
        {
            printf( "socket failed: %s\n", strerror( errno ));
            return;
        }

        siz = 1;
        (void)setsockopt( fd, SOL_SOCKET, SO_REUSEADDR,
                          (setsockoptcast) &fd, sizeof( fd ));

        sa.sin_family = AF_INET;
        sa.sin_port = htons(port);
        sa.sin_addr.s_addr = INADDR_ANY;

        if ( bind( fd, (struct sockaddr *)&sa, sizeof( sa )) < 0 )
        {
            printf( "bind port %d failed: %s\n", port, strerror( errno ));
            return;
        }

        listen( fd, 10 );

        c = new conn( fd, remote_host, remote_port );
        conns.add( c );
    }

    if ( register_mq( mq, "fdactive" ) == false )
    {
        printf( "can't register mq\n" );
        return;
    }

    union {
        Message          * m;
        FdActiveMessage  * fam;
        MainTimer        * mt;
    } m;
    MainTimer * mt;
    int mqout;

    for ( c = conns.get_head(); c; c = conns.get_next(c) )
    {
        register_fd_mq( c->fds[0], c, FOR_READ, mq );
        if ( !c->listen )
            register_fd_mq( c->fds[1], c, FOR_READ, mq );
    }

    mt = new MainTimer;
    mt->dest.set( mq );
    set( tps(), mt );

    while ( 1 )
    {
        m.m = recv( 1, &mq, &mqout, WAIT_FOREVER );

        switch ( m.m->type.get() )
        {
        case FdActiveMessage::TYPE:
            handle_fd( m.fam->fd.get(), (conn*)m.fam->arg );
            break;

        case MainTimer::TYPE:
            mt = new MainTimer;
            mt->dest.set( mq );
            set( tps(), mt );
            for ( c = conns.get_head(); c; c = conns.get_next(c) )
                printf( "conn %d->%d bytes %d\n",
                        c->fds[0], c->fds[1], c->bytes );
            printf( "total bytes: %d\n", bytes );
            break;

        default:
            printf( "unknown message %#x\n", m.m->type.get() );
            break;
        }

        delete m.m;
    }
}

void
MainThread :: handle_fd( int fd, conn * c )
{
    int rind = 1, wind = 0;

    if ( fd == c->fds[0] )
        rind = 0, wind = 1;

    if ( c->listen )
    {
        conn * c2;
        struct sockaddr_in sa;
        socklen_t salen = sizeof( sa );
        int ear, nfd;

        ear = accept( c->fds[0], (struct sockaddr *)&sa, &salen );
        if ( ear < 0 )
        {
            printf( "accept: %s\n", strerror( errno ));
        }
        else
        {
            nfd = socket( AF_INET, SOCK_STREAM, 0 );
            if ( nfd < 0 )
            {
                printf( "socket call failed: %s\n", strerror( errno ));
                close( ear );
            }
            else
            {
                //xxx not async
                sa.sin_family = AF_INET;
                sa.sin_port = htons( c->port );
                sa.sin_addr = c->addr;
                if ( connect( nfd, (struct sockaddr *)&sa, sizeof( sa )) < 0 )
                {
                    printf( "connect: %s\n", strerror( errno ));
                    close( ear );
                    close( nfd );
                }
                else
                {
                    c2 = new conn( ear, nfd );
                    conns.add( c2 );
                    register_fd_mq( c2->fds[0], c2, FOR_READ, mq );
                    register_fd_mq( c2->fds[1], c2, FOR_READ, mq );
                }
            }
        }
        register_fd_mq( c->fds[rind], c, FOR_READ, mq );
    }
    else
    {
        char buf[1024];
        int cc = read( c->fds[rind], buf, 1024 );
        if ( cc <= 0 )
        {
            conns.remove( c );
            unregister_fd_mq( c->fds[wind] );
            delete c;
        }
        else
        {
            write( c->fds[wind], buf, cc );
            c->bytes += cc;
            bytes += cc;
            register_fd_mq( c->fds[rind], c, FOR_READ, mq );
        }
    }
}
