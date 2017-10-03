#if 0
g++ -Dtcpgate2_main=main -Ithreads/h tcpgate2.C threads/libthreads.a -o gate2
exit 0
#endif

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

#include "threads.H"

#if defined(SUNOS) || defined(SOLARIS) || defined(CYGWIN)
#define socklen_t int
#define setsockoptcast char*
#else
#define setsockoptcast void*
#endif

extern "C" int tcpgate2_main( int argc, char ** argv );


class Listener : public Thread {
    void entry( void );
    int ear_fd;
    int my_port;
    char * remote_hostname;
    int remote_port;
    struct in_addr remote_host;
public:
    Listener( int _port, char * _host, int _remote_port )
        : Thread( "Listen", 10, 32 * 1024 ) {
        my_port = _port;
        int hostlen = strlen( _host ) + 1;
        remote_hostname = new char[ hostlen ];
        memcpy( remote_hostname, _host, hostlen );
        remote_port = _remote_port;
        resume( tid );
    }
    ~Listener( void ) {
        close( ear_fd );
        delete[] remote_hostname;
    }
};

class DataMgr : public Thread {
    void entry( void );
    int ear;
    int fds[2];
    struct in_addr remote_host;
    int remote_port;
    char * remote_hostname;
public:
    DataMgr( int _ear, char * _remote_hostname,
             struct in_addr _remote_host, int _remote_port )
        : Thread( "Data", 11, 32 * 1024 ) {
        ear = _ear;
        fds[0] = fds[1] = -1;
        remote_host = _remote_host;
        remote_port = _remote_port;
        remote_hostname = _remote_hostname;
        resume( tid );
    }
    ~DataMgr( void ) {
        printf( "closing %d and %d\n", fds[0], fds[1] );
        if ( fds[0] > 0 ) close( fds[0] );
        if ( fds[1] > 0 ) close( fds[1] );
    }
};

int
tcpgate2_main( int argc, char ** argv )
{
    ThreadParams p;
    p.my_eid = 1;
    p.max_threads = 512;
    p.max_fds = 1024;

    Threads th( &p );

    argc -= 1;
    argv += 1;

    while ( argc != 0 )
    {
        int port, remote_port;
        char * host;

        if ( argc < 3 )
        {
            fprintf( stderr, "arg parsing problem\n" );
            return 1;
        }

        port        = atoi( argv[0] );
        host        =       argv[1];
        remote_port = atoi( argv[2] );

        argc -= 3;
        argv += 3;

        (void) new Listener( port, host, remote_port );
    }

    th.loop();

    return 0;
}

void
Listener :: entry( void )
{
    printf( "managing local port %d to remote host %s port %d\n",
            my_port, remote_hostname, remote_port );

    struct sockaddr_in sa;

    if ( inet_aton( remote_hostname, &sa.sin_addr ))
    {
        remote_host = sa.sin_addr;
    }
    else
    {
        struct hostent *he;
        if (( he = gethostbyname( remote_hostname )) == NULL )
        {
            printf( "host %s is not found!\n", remote_hostname );
            return;
        }
        memcpy( &remote_host, he->h_addr, 4 );
    }

    ear_fd = socket( AF_INET, SOCK_STREAM, 0 );

    if ( ear_fd < 0 )
    {
        printf( "socket failed: %s\n", strerror( errno ));
        return;
    }

    int siz = 1;
    (void)setsockopt( ear_fd, SOL_SOCKET, SO_REUSEADDR,
                      (setsockoptcast) &siz, sizeof( siz ));

    register_fd( ear_fd );

    sa.sin_family = AF_INET;
    sa.sin_port = htons(my_port);
    sa.sin_addr.s_addr = INADDR_ANY;

    if ( bind( ear_fd, (struct sockaddr *)&sa, sizeof( sa )) < 0 )
    {
        printf( "bind on port %d failed: %s\n", my_port,
                strerror( errno ));
        return;
    }

    listen( ear_fd, 10 );

    int selout;

    while ( 1 )
    {
        int selret = select( 1, &ear_fd, 0, 0, 1, &selout, WAIT_FOREVER );
        if ( selret > 0 )
        {
            new DataMgr( ear_fd, remote_hostname,
                         remote_host, remote_port );
        }
    }
}

void
DataMgr :: entry( void )
{
    struct sockaddr_in sa;
    socklen_t salen = sizeof( sa );
    fds[0] = accept( ear, (struct sockaddr*)&sa, &salen );
    int e = errno;

    printf( "accept fd %d err %d to host %s port %d\n",
            fds[0], e, remote_hostname, remote_port );

    if ( fds[0] < 0 )
    {
        printf( "accept failed: %s\n", strerror( e ));
        return;
    }

    fds[1] = socket( AF_INET, SOCK_STREAM, 0 );

    sa.sin_family = AF_INET;
    sa.sin_port = htons( remote_port );
    sa.sin_addr = remote_host;

    register_fd( fds[0] );
    register_fd( fds[1] );

    if ( connect( fds[1], (struct sockaddr *)&sa, sizeof( sa )) < 0 )
    {
        if ( errno != EINPROGRESS )
        {
            printf( "connect failed: %s\n", strerror( errno ));
            return;
        }
    }

    int selout[2], selret;

    selret = select( 1, fds+1, 1, fds+1, 2, selout, WAIT_FOREVER );

    salen = sizeof( sa );
    if ( getpeername( fds[1], (struct sockaddr *)&sa, &salen ) < 0 )
    {
        if ( errno == ENOTCONN )
        {
            printf( "connect failed\n" );
            return;
        }
    }

    char * buf = new char[ 1024 ];
    bool dead = false;

    while ( !dead )
    {
        selret = select( 2, fds, 0, 0, 2, selout, WAIT_FOREVER );
        if ( selret < 0 )
        {
            printf( "select: %s\n", strerror( errno ));
            break;
        }
        while ( selret-- > 0 )
        {
            int cc;
            if ( selout[selret] == fds[0] )
            {
                cc = read( fds[0], buf, 1024 );
                if ( cc <= 0 )
                    dead = true;
                else
                    write( fds[1], buf, cc );
            }
            else
            {
                cc = read( fds[1], buf, 1024 );
                if ( cc <= 0 )
                    dead = true;
                else
                    write( fds[0], buf, cc );
            }
        }
    }

    delete[] buf;
}
