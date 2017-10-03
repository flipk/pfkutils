/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

#include "adm_hookup.H"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#if defined(SUNOS) || defined(SOLARIS)
#define socklen_t int
#define setsockoptcast char*
#else
#define setsockoptcast void*
#endif

Adm_Hookup_fd :: Adm_Hookup_fd( Adm_Hookup_Factory_iface * _factory,
                                short port, char * _dest_host,
                                short _dest_port )
{
    int siz;
    struct sockaddr_in sa;
    struct hostent * he;

    factory = _factory;

    sa.sin_family = AF_INET;
    sa.sin_port = htons( port );
    sa.sin_addr.s_addr = INADDR_ANY;

    dest_sa = new sockaddr_in;
    *dest_sa = sa;

    if ( !(inet_aton( _dest_host, &dest_sa->sin_addr )))
    {
        if (( he = gethostbyname( _dest_host )) == NULL )
        {
            fprintf( stderr, "unknown host: %s\n", _dest_host );
            exit( 1 );
        }
        memcpy( &dest_sa->sin_addr, he->h_addr, he->h_length );
    }
    dest_sa->sin_port = htons( _dest_port );

    fd = socket( AF_INET, SOCK_STREAM, 0 );
    if ( fd < 0 )
    {
        fprintf( stderr, "socket: %s\n", strerror( errno ));
        exit( 1 );
    }

    siz = 64 * 1024;
    (void)setsockopt(fd, SOL_SOCKET, SO_RCVBUF,
                     (setsockoptcast) &siz, sizeof(siz));
    (void)setsockopt(fd, SOL_SOCKET, SO_SNDBUF,
                     (setsockoptcast) &siz, sizeof(siz));
    siz = 1;
    (void)setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
                     (setsockoptcast) &siz, sizeof(siz));

    if ( bind( fd, (struct sockaddr *)&sa, sizeof(sa)) < 0 )
    {
        fprintf( stderr, "bind: %s\n", strerror( errno ));
        exit( 1 );
    }

    listen( fd, 1 );
}

Adm_Hookup_fd :: ~Adm_Hookup_fd( void )
{
    delete dest_sa;
    close( fd );
}

//virtual
fd_interface::rw_response
Adm_Hookup_fd :: read( fd_mgr * fdmgr )
{
    int ear, salen, outfd;
    struct sockaddr_in sa;

    salen = sizeof( struct sockaddr_in );
    ear = accept( fd, (struct sockaddr *)&sa, (socklen_t*)&salen );

    if ( ear < 0 )
    {
        fprintf( stderr, "accept: %s\n", strerror( errno ));
        return OK;
    }

    outfd = socket( AF_INET, SOCK_STREAM, 0 );
    if ( outfd < 0 )
    {
        fprintf( stderr, "socket: %s\n", strerror( errno ));
        close( ear );
        return OK;
    }

    fcntl( outfd, F_SETFL, 
           fcntl( outfd, F_GETFL, 0 ) | O_NONBLOCK );

    if ( connect( outfd, (struct sockaddr *)dest_sa, sizeof (*dest_sa) ) < 0 )
    {
        if ( errno != EINPROGRESS )
        {
            fprintf( stderr, "connect: %s\n", strerror( errno ));
            close( ear );
            close( outfd );
            return OK;
        }
    }

    unsigned long addr = htonl( dest_sa->sin_addr.s_addr );
    printf( "connection attempt in progress to %d.%d.%d.%d port %d\n",
            (addr >> 24) & 0xff, (addr >> 16) & 0xff,
            (addr >>  8) & 0xff, (addr >>  0) & 0xff,
            htons( dest_sa->sin_port ));

    factory->new_gateway( ear, outfd, fdmgr );

    // now that an incoming telnet and outgoing connection
    // are hooked up to each other, the listening port should
    // be closed.

    return DEL;
}

//virtual
void
Adm_Hookup_fd :: select_rw ( fd_mgr *, bool * rd, bool * wr )
{
    // always select for read to look for new connections
    *rd = true;
    *wr = false;
}
