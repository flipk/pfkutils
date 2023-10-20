
#include "main.H"
#include "threads.H"
#include "client.H"

#include <stdarg.h>

Client :: Client( int _fd, unsigned int addr )
    : Thread( "Client", MY_PRIO )
{
    char * a = (char*) &addr;
    fd_user = _fd;
    stdio();
    printf( "Client: new client on fd %d from %d.%d.%d.%d\n",
            fd_user, a[0], a[1], a[2], a[3] );
    unstdio();
    fd_server = -1;
    resume( tid );
}

Client :: ~Client( void )
{
    stdio();
    printf( "Client for fd %d <--> fd %d is exiting\n",
            fd_user, fd_server );
    unstdio();
    close( fd_user );
    if ( fd_server != -1 )
        close( fd_server );
}

void
Client :: print( char * format, ... )
{
    char obuf[ linelen ];
    va_list ap;
    int len;

    // xxx: vsnprintf doesn't return an int on some platforms
    va_start( ap, format );
    len = vsnprintf( obuf, linelen-1, format, ap );
    va_end( ap );

    obuf[ linelen-1 ] = 0;
    (void) write( fd_user, obuf, len );
}

void
Client :: entry( void )
{
    fd_server = socket( AF_INET, SOCK_STREAM, 0 );
    if ( fd_server < 0 )
    {
        int e = errno;
        stdio();
        printf( "client: socket failed (%s)\n", strerror( e ));
        unstdio();
        print( "client: socket failed (%s)\n", strerror( e ));
        fd_server = -1;
        return;
    }

    struct sockaddr_in sa;

    sa.sin_family = AF_INET;
    sa.sin_port = htons( 23 );
    sa.sin_addr.s_addr = htonl( 0x7f000001 );

    if ( connect( fd_server, (struct sockaddr *)&sa, sizeof( sa )) < 0 )
    {
        int e = errno;
        stdio();
        printf( "client: socket failed (%s)\n", strerror( e ));
        unstdio();
        print( "client: socket failed (%s)\n", strerror( e ));
        return;
    }

    static const int bufsize = 512;
    char buf[ bufsize ];

    while ( global_exit == false )
    {
        int fds[2];
        int ofds[2];
        int cc;

        fds[0] = fd_user;
        fds[1] = fd_server;

        cc = select( 2, fds, 0, 0, 2, ofds, WAIT_FOREVER );
        while ( cc-- > 0 )
        {
            int cc2;
            if ( ofds[cc] == fd_user )
            {
                cc2 = read( fd_user, buf, bufsize );
                if ( cc2 <= 0 )
                    return;
                log( log_user, buf, cc2 );
                (void) write( fd_server, buf, cc2 );
            }
            else if ( ofds[cc] == fd_server )
            {
                cc2 = read( fd_server, buf, bufsize );
                if ( cc2 <= 0 )
                    return;
                log( log_server, buf, cc2 );
                (void) write( fd_user, buf, cc2 );
            }
        }
    }
}

void
Client :: log( int which, char * _buf, int len )
{
    int i, j;
    unsigned char * buf = (unsigned char *)_buf;
    stdio();

    printf( "Client %d <--> %d %s:\n", 
            fd_user, fd_server, 
            (which == log_user) ? "user" : "server" );

    for ( i = 0; i < len; i += 16 )
    {
        for ( j = 0; j < 16; j++ )
        {
            if ( (j&3) == 0 )
                printf( " " );
            if ( (i+j) < len )
                printf( "%02x ", buf[i+j] );
            else
                printf( "   " );
        }
        printf( "    " );
        for ( j = 0; j < 16; j++ )
        {
            if ( (i+j) < len )
            {
                unsigned char c = buf[i+j];
                if ( c < 32 || c > 127 )
                    c = '.';
                printf( "%c", c );
            }
            else
            {
                printf( " " );
            }
        }
        printf( "\n" );
    }
    printf( "\n" );
    unstdio();
}
