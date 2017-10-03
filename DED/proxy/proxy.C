
#include "config.h"
#include "proxy.H"
#include "inputbuf.H"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

linked_list<proxyThread> * proxys;

class mime_headers {
    int num;
    int alloc;
    char ** l;
public:
    mime_headers( void );
    ~mime_headers( void );
    void erase( void );
    int getnum( void ) { return num; }
    void add( char * );
    char * get( int n ) { if ( n < num ) return l[n]; return NULL; }
    void dump( void );
};

mime_headers :: mime_headers( void )
{
    num = 0;
    alloc = 0;
    l = NULL;
}

mime_headers :: ~mime_headers( void )
{
    for ( int i = 0; i < num; i++ )
        delete[] l[i];
    if ( l )
        delete[] l;
}

void
mime_headers :: erase( void )
{
    for ( int i = 0; i < num; i++ )
        delete[] l[i];
    num = 0;
    // preserve alloc array
}

void
mime_headers :: add( char * x )
{
    if ( num == alloc )
    {
        char ** old = l;
        int oalloc = alloc;
        alloc += 20;
        l = new char*[ alloc ];
        if ( old )
        {
            memcpy( l, old, sizeof(char*) * oalloc );
            delete[] old;
        }
    }
    l[num++] = x;
}

void
mime_headers :: dump( void )
{
    for ( int i = 0; i < num; i++ )
        printf( "%s\n", l[i] );
}

proxyThread :: proxyThread( int _fd )
    : Thread( "proxy", myprio, mystacksize )
{
    fd = _fd;

    outbuf = NULL;
    inrequest = NULL;
    outrequest = NULL;
    outfd = -1;
    my_url = NULL;
    death_requested = false;

    inbuf = new( mybufsize ) inputbuf( (int)this, &inbufreadfunc );
    inmh = new mime_headers;
    outmh = new mime_headers;
    log = new connLog;

    proxys->add( this );

    log->print( "proxyThread created, fd = %d\n", fd );

    resume( tid );
}

proxyThread :: ~proxyThread( void )
{
    proxys->del( this );
    if ( my_url )
    {
        log->print( "DONE (log %d) %s\n", get_lognumber(), my_url );
        printf( "DONE (log %d) %s\n", get_lognumber(), my_url );
        delete[] my_url;
    }
    close( fd );
    if ( outfd != -1 )
        close( outfd );
    req_cleanup();
    delete inbuf;
    if ( outbuf )
        delete outbuf;
    log->print( "proxyThread exiting\n" );
    delete log;
}

//static
int
proxyThread :: inbufreadfunc( int pseudofd, char * buf, int len )
{
    int r;
    proxyThread * pt = (proxyThread *)pseudofd;
    r = pt->read( pt->fd, buf, len );
    if ( r > 0 )
        pt->log->logdat( connLog::FROM_BROWSER, buf, r );
    return r;
}

//static
int
proxyThread :: outbufreadfunc( int pseudofd, char * buf, int len )
{
    int r;
    proxyThread * pt = (proxyThread *)pseudofd;
    r = pt->read( pt->outfd, buf, len );
    if ( r > 0 )
        pt->log->logdat( connLog::FROM_SITE, buf, r );
    return r;
}

int
proxyThread :: mywrite( int writefd, char * buf, int len )
{
    int ret;
    ret = write( writefd, (void*)buf, len );
    if ( writefd == fd )
        log->logdat( connLog::TO_BROWSER, buf, len );
    else
        log->logdat( connLog::TO_SITE, buf, len );
    return ret;
}

void
proxyThread :: entry( void )
{
    register_fd( fd );
#ifdef KEEP_ALIVE
    static const char * kas = "Proxy-Connection: Keep-Alive";
#else
    static const char * kas = "Proxy-Connection: Close";
#endif
    char * ka;
    int kaslen = strlen( kas ) + 1;

    do {
        keepalive = false;

        if ( get_request() < 0 )
            return;
        if ( get_mimehdrs( inbuf, inmh ) < 0 )
            return;
        if ( parse_req() < 0 )
            return;
        if ( open_proxy_conn() < 0 )
            return;
        if ( send_request() < 0 )
            return;
        if ( get_mimehdrs( outbuf, outmh ) < 0 )
            return;
        if ( parse_outmime() < 0 )
            return;
        ka = NULL;
        if ( keepalive )
        {
            log->print( "adding kas to mime sent to browser\n" );
            ka = new char[ kaslen ];
            strcpy( ka, kas );
            outmh->add( ka );
#ifndef KEEP_ALIVE
            keepalive = false;
#endif
        }
        if ( pass_data() < 0 )
            return;

        req_cleanup();

    } while ( keepalive );
}

int
proxyThread :: get_request( void )
{
    inrequest = inbuf->getline();
    if ( inrequest == NULL )
        return -1;

    log->print( "getline : %s\n", inrequest );

    if ( strncmp( inrequest, "GET ", 4 ) == 0 )
        return 0;

    if ( strncmp( inrequest, "POST ", 5 ) == 0 )
        return 0;

    if ( strncmp( inrequest, "CONNECT ", 8 ) == 0 )
    {
        printf( "UNSUPPORTED : %s\n", inrequest );
    }

    return -1;
}

int
proxyThread :: get_mimehdrs( inputbuf * buf, mime_headers * mh )
{
    while ( 1 )
    {
        char * b = buf->getline();
        if ( !b )
        {
            log->print( "get_mimeheaders: getline return null\n" );
            return -1;
        }
        log->print( "get_mimeheaders: got line '%s'\n", b );
        if ( b[0] == 0 )
        {
            delete[] b;
            return 0;
        }
        mh->add( b );
    }
    // NOTREACHED
}

int
proxyThread :: parse_req( void )
{
    int i;
    char * l;

    in_content_length = -1;

    for ( i = 0; l = inmh->get(i); i++ )
    {
        char conntype[32];
        int new_connlen;

        log->print( "parse_req: checking header '%s'\n", l );

        if ( sscanf( l, "Proxy-Connection: %32s", &conntype ) == 1 )
        {
            if ( strcmp( conntype, "Keep-Alive" ) == 0 )
            {
                log->print( "parse_req: found keepalive\n" );
                keepalive = true;
            }
        }
        else if ( sscanf( l, "Content-Length: %d", &new_connlen ) == 1 )
        {
            log->print( "parse_req: content length is %d\n", new_connlen );
            in_content_length = new_connlen;
        }
    }

    // trim host from request string
    // set outrequest

    char cmd[8];
    char URL[maxurlsize];
    char version[32];

    if ( strlen( inrequest ) > ( 8 + maxurlsize + 32 ))
    {
        log->print( "inrequest is just too long to parse\n" );
        printf( "inrequest is just too long to parse\n" );
        return -1;
    }

    if ( sscanf( inrequest, "%8s %s %32s", 
                 &cmd, &URL, &version ) != 3 )
    {
        log->print( "inrequest parse error\n" );
        printf( "inrequest parse error\n" );
        return -1;
    }

    // separate proto, host, and path

    if ( my_url )
    {
        log->print( "DONE (log %d) %s\n", get_lognumber(), my_url );
        printf( "DONE (log %d) %s\n", get_lognumber(), my_url );
        delete[] my_url;
    }

    if ( strlen( URL ) > 130 )
    {
        my_url = new char[ 130 ];
        memcpy( my_url, URL, 120 );
        sprintf( my_url + 120, " [more]" );
    }
    else
    {
        my_url = new char[ strlen( URL ) + 1 ];
        strcpy( my_url, URL );
    }

    log->print( "START (log %d) %s\n", get_lognumber(), my_url );
    printf( "START (log %d) %s\n", get_lognumber(), my_url );

    char * proto;
    char * host;
    char * path;
    char * p;

    proto = URL;
    for ( p = proto; *p; p++ )
        if ( *p == ':' )
            break;

    if ( *p == 0 )
    {
        log->print( "URL parse error 1\n" );
        printf( "URL parse error 1\n" );
        return -1;
    }

    *p = 0;

    if ( p[1] != '/' || p[2] != '/' )
    {
        log->print( "URL parse error 2\n" );
        printf( "URL parse error 2\n" );
        return -1;
    }

    remoteport = -1;
    host = p + 3;
    for ( p += 3; *p != 0 && *p != '/'; p++ )
        if ( *p == ':' )
        {
            *p++ = 0;
            remoteport = atoi( p );
        }

    if ( *p == 0 )
    {
        log->print( "URL parse error 3\n" );
        printf( "URL parse error 3\n" );
        return -1;
    }

    *p++ = 0;
    path = p;

    if ( remoteport == -1 )
    {
        // set default remoteport based on proto

#define CMP(x) if ( strcmp( proto, x ) == 0 ) 

        CMP("http")
            {
                remoteport = 80;
            }

        CMP("https")
            {
                remoteport = 443;
            }
    }

#undef  CMP

    if ( remoteport == -1 )
    {
        log->print( "XXX: why is port # -1? (proto=%s)\n", proto );
        printf( "XXX: why is port # -1? (proto=%s)\n", proto );
        return -1;
    }

    remotehost = new char[ strlen( host ) + 1 ];
    strcpy( remotehost, host );

    log->print( "proto : %s\n"
                "host : %s\n"
                "path : %s\n"
                "remotehost : %s\n"
                "remoteport : %d\n",
                proto, host, path, remotehost, remoteport );

    // construct outrequest

    int outlen = 
        strlen( cmd ) + 1 +
        strlen( path ) + 1 +
        strlen( version ) + 4;

    outrequest = new char[ outlen ];
    sprintf( outrequest, "%s /%s %s\r\n", cmd, path, version );

    log->print( "outlen = %d\n"
                "real outlen = %d\n"
                "outrequest: %s\n",
                outlen,
                strlen( outrequest ),
                outrequest );

    return 0;
}

int
proxyThread :: open_proxy_conn( void )
{
    outfd = socket( AF_INET, SOCK_STREAM, 0 );

    if ( outfd < 0 )
    {
        printf( "socket error: %s\n", strerror( errno ));
        return -1;
    }

    struct sockaddr_in sa;

    sa.sin_family = AF_INET;
    sa.sin_port = htons( (short)remoteport );

    if ( !inet_aton( remotehost, &sa.sin_addr ))
    {
        struct hostent * he;
        if (( he = gethostbyname( remotehost )) == NULL )
        {
            printf( "unknown host '%s'\n", remotehost );
            return -1;
        }
        memcpy( &sa.sin_addr, he->h_addr, he->h_length );
    }

    if ( connect( outfd, (struct sockaddr *)&sa, sizeof sa ) < 0 )
    {
        log->print( "connect error: %s\n", strerror( errno ));
        printf( "connect error: %s\n", strerror( errno ));
        return -1;
    }

    log->print( "connected with fd %d\n", outfd );
    register_fd( outfd );

    outbuf = new( mybufsize ) inputbuf( (int)this, &outbufreadfunc );
    return 0;
}

int
proxyThread :: send_request( void )
{
    // trim "Proxy-Connection:" and "Connection:" from headers sent!

    if ( mywrite( outfd, outrequest, strlen( outrequest )) <= 0 )
        return -1;

    send_mime( outfd, inmh );

    // if its a POST connection, send the POST data

    if ( in_content_length != -1 )
    {
        char buf[ mybufsize ];
        while ( in_content_length > 0 )
        {
            int len = (in_content_length > mybufsize) ?
                mybufsize : in_content_length;
            int r;

            r = inbuf->read( buf, len );
            log->print( "send_request: read %d -> %d from input\n",
                        len, r );
            if ( r <= 0 )
                return -1;
            if ( mywrite( outfd, buf, r ) <= 0 )
                return -1;
            in_content_length -= r;
        }
    }

    return 0;
}

int
proxyThread :: parse_outmime( void )
{
    char * l;
    int nlen;
    out_content_length = -1;
    for ( int i = 0; l = outmh->get( i ); i++ )
    {
        if ( sscanf( l, "Content-Length: %d", &nlen ) == 1 )
        {
            log->print( "parse_outmime: content length %d\n", nlen );
            out_content_length = nlen;
        }
    }
    return 0;
}

void
proxyThread :: send_mime( int myfd, mime_headers * mh )
{
    char * l, * out, * p;
    int len, i;

    len = 0;
    for ( i = 0; l = mh->get( i ); i++ )
    {
        if ( mh == inmh )
            if ( strncmp( l, "Proxy-Connection:",
                          sizeof( "Proxy-Connection:" ) - 1 ) == 0 )
                // skip
                continue;
        if ( mh == outmh )
            if ( strncmp( l, "Connection:",
                          sizeof( "Connection:" ) - 1 ) == 0 )
                // skip
                continue;

        len += strlen( l );
        len += 2;
    }
    len += 2;

    out = new char[ len ];
    p = out;

    for ( i = 0; l = mh->get( i ); i++ )
    {
        if ( mh == inmh )
            if ( strncmp( l, "Proxy-Connection:",
                          sizeof( "Proxy-Connection:" ) - 1 ) == 0 )
                // skip
                continue;
        if ( mh == outmh )
            if ( strncmp( l, "Connection:",
                          sizeof( "Connection:" ) - 1 ) == 0 )
                // skip
                continue;

        int ll = strlen( l );
        memcpy( p, l, ll );
        p += ll;
        *p++ = '\r';
        *p++ = '\n';
    }
    *p++ = '\r';
    *p++ = '\n';

    mywrite( myfd, out, len );
    delete[] out;
}

int
proxyThread :: pass_data( void )
{
    char * l;

    send_mime( fd, outmh );
    char buf[mybufsize];

    if ( out_content_length == -1 )
    {
        while ( 1 )
        {
            int r = outbuf->read( buf, mybufsize );
            log->print( "pass_data: read1 %d -> %d\n",
                        mybufsize, r );
            if ( r <= 0 )
                return -1;
            if ( mywrite( fd, buf, r ) <= 0 )
                return -1;
        }
    }
    /* else */

    int r;

    r = outbuf->data_left();
    if ( r > 0 )
    {
        log->print( "pass_data: outbuf had %d bytes left\n", r );
        outbuf->read( buf, r );
        mywrite( fd, buf, r );
        out_content_length -= r;
    }

    r = inbuf->data_left();
    if ( r > 0 )
    {
        log->print( "pass_data: inbuf had %d bytes left\n", r );
        inbuf->read( buf, r );
        mywrite( outfd, buf, r );
        in_content_length -= r;
    }

    while ( in_content_length != 0 && out_content_length != 0 )
    {
        int rfds[2] = { fd, outfd };
        int ofd;
        log->print( "pass_data: block in select\n" );
        r = select( 2, rfds, 0, NULL, 1, &ofd, WAIT_FOREVER );
        if ( death_requested )
        {
            log->print( "proxyThread exiting due to death request\n" );
            return -1;
        }
        log->print( "pass_data: select awoke, r %d ofd %d\n", r, ofd );
        if ( r == 1 )
        {
            int len = mybufsize;
            if ( ofd == outfd )
            {
                if ( out_content_length > 0 )
                    len = (out_content_length > mybufsize) ? 
                        mybufsize : out_content_length;
                r = read( outfd, buf, len );
                log->print( "pass_data: read outfd %d -> %d\n", len, r );
                if ( r <= 0 )
                    return -1;
                mywrite( fd, buf, r );
                out_content_length -= r;
            }
            else
            {
                if ( in_content_length > 0 )
                    len = (in_content_length > mybufsize) ?
                        mybufsize : in_content_length;
                r = read( fd, buf, len );
                log->print( "pass_data: read fd %d -> %d\n", len, r );
                if ( r <= 0 )
                    return -1;
                mywrite( outfd, buf, r );
                in_content_length -= r;
            }
        }
        log->print( "pass_data: inlen %d outlen %d\n",
                    in_content_length, out_content_length );
    }


    log->print( "pass_data: done\n" );

    return 0;
}

void
proxyThread :: req_cleanup( void )
{

#define DEL(x) do { if (x) { delete[] x; x = NULL; } } while( 0 )

    log->print( "req_cleanup\n" );
    close( outfd );

    if ( my_url )
    {
        strcpy( my_url, "(keep-alive)" );
    }

    outfd = -1;
    DEL(inrequest);
    DEL(outrequest);
    if ( outbuf )
    {
        delete outbuf;
        outbuf = NULL;
    }
    inmh->erase();
    outmh->erase();

#undef  DEL

}
