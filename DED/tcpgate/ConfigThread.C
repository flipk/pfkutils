
#include "threads.H"
#include "AcceptorThread.H"
#include "ConfigThread.H"
#include "DataThread.H"

#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <string.h>

extern "C" int inet_aton( register const char *cp, struct in_addr *addr );


//
// config file format:
//
//    manual <port>
//    status <port>
//    map <port> <host> <port>
//    [map....]
//

const char * ConfigFile :: config_file_name = ".tcpgate";

struct config {
    struct config * next;
    int port;
    int remotehost;
    int remoteport;
};

static config * configs;

static void
add_config( int p, int rh, int rp )
{
    config * x = new config;
    x->port = p;
    x->remotehost = rh;
    x->remoteport = rp;
    x->next = configs;
    configs = x;
}

static int
gethost( char * buf )
{
    struct sockaddr_in sa;
    int raddr;

    if ( inet_aton( buf, &sa.sin_addr ))
    {
        memcpy( &raddr, &sa.sin_addr, 4 );
    }
    else
    {
        struct hostent * he = gethostbyname( buf );
        if ( !he )
        {
            printf( "host is not found.\n\n" );
            return 0;
        }
        memcpy( &raddr, he->h_addr, 4 );
    }

    raddr = htonl( raddr );

    return raddr;
}

ConfigFile :: ConfigFile( void )
{
    char line[ 128 ];
    FILE * f;

    ok = false;
    configs = NULL;

    sprintf( line, "%s/%s", getenv( "HOME" ), config_file_name );
    f = fopen( line, "r" );
    if ( !f )
    {
        fprintf( stderr, "cannot open config file %s\n", line );
        return;
    }

#define CHK(s) if ( strncmp( line, #s " ", sizeof( #s )) == 0 )

    while ( fgets( line, sizeof( line ), f ))
    {
        CHK(manual)
            {
                add_config( atoi( line+7 ), AcceptorThread::CONFIG_HOST, 0 );
            }
        CHK(status)
            {
                add_config( atoi( line+7 ), AcceptorThread::STATUS_HOST, 0 );
            }
        CHK(map)
            {
                char * p, * rh, * rp;
                int ip, irh, irp;
                p = line+4;
                rh = strchr( p, ' ' );
                if ( !rh )
                {
                    fprintf( stderr, "<map> error\n" );
                    exit( 1 );
                }
                *rh++ = 0;
                rp = strchr( rh+1, ' ' );
                if ( !rp )
                {
                    fprintf( stderr, "<map> error\n" );
                    exit( 1 );
                }
                *rp++ = 0;

                ip = atoi( p );
                irh = gethost( rh );
                irp = atoi( rp );

                if ( ip == 0 || irh == 0 || irp == 0 )
                {
                    fprintf( stderr, "<map> error\n" );
                    exit( 1 );
                }

                add_config( ip, irh, irp );
            }
    }

    fclose( f );
    ok = true;
}

ConfigFile :: ~ConfigFile( void )
{
    config * x, * nx;
    for ( nx = NULL, x = configs; x; x = nx )
    {
        nx = x->next;
        delete x;
    }
}

void
ConfigFile :: create_threads( void )
{
    config * x;
    for ( x = configs; x; x = x->next )
    {
        new AcceptorThread( x->port, x->remotehost, x->remoteport );
    }
}

ConfigThread :: ConfigThread( int _fd )
    : Thread( "Config", myprio, mystack )
{
    fd = _fd;
    fdfw = fdopen( fd, "w" );
    register_fd( fd );
    resume( tid );
}

ConfigThread :: ~ConfigThread( void )
{
    fclose( fdfw );
}

void
ConfigThread :: vprintf( char * format, va_list ap )
{
    vfprintf( fdfw, format, ap );
}

void
ConfigThread :: printf( char * format, ... )
{
    va_list ap;

    va_start( ap, format );
    vprintf( format, ap );
    va_end( ap );
}

void
ConfigThread :: flush( void )
{
    fflush( fdfw );
}

bool
ConfigThread :: getline( char * buf, int len )
{
    int i;
    buf[len-1] = 0;
    len--;

    while ( len > 0 )
    {
        int cc;
        cc = read( fd, buf, len );
        if ( cc <= 0 )
            return false;
        for ( i = 0; i < cc; i++ )
            if ( buf[i] == '\r' || buf[i] == '\n' )
            {
                buf[i] = 0;
                return true;
            }
        len -= cc;
        buf += cc;
    }

    return true;
}

void
ConfigThread :: entry( void )
{
    char buf[ 80 ];
    int cc;

    printf( "\n\n   Connect to host: " );
    flush();

    if ( getline( buf, 80 )  == false )
        return;

    char * colon = strchr( buf, ':' );
    if ( colon != NULL )
        *colon++ = 0;

    int rport = 23;
    if ( colon )
        rport = atoi( colon );

    int raddr = gethost( buf );

    if ( raddr == 0 )
    {
        printf( "unable to resolve hostname\n" );
        return;
    }
    printf( "Connecting to host %s at %d.%d.%d.%d port %d\n",
            buf, (raddr >> 24) & 0xff, (raddr >> 16) & 0xff, 
            (raddr >> 8) & 0xff, (raddr) & 0xff, rport );
    flush();

    (void) new DataThread( dup( fd ), 0, 0, raddr, rport );
    return;
}
