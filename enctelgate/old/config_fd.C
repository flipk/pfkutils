
#include "config_fd.H"
#include "config_messages.H"

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


Config_fd :: Config_fd( Pipe_Mgr * _pipe_mgr, short udp_port_no )
{
    pipe_mgr = _pipe_mgr;
    struct sockaddr_in sa;

    sa.sin_family = AF_INET;
    sa.sin_port = htons( udp_port_no );
    sa.sin_addr.s_addr = INADDR_ANY;

    fd = socket( AF_INET, SOCK_DGRAM, 0 );
    if ( fd < 0 )
    {
        fprintf( stderr, "socket: %s\n", strerror( errno ));
        exit( 1 );
    }

    if ( bind( fd, (struct sockaddr *)&sa, sizeof( sa )) < 0 )
    {
        fprintf( stderr, "bind: %s\n", strerror( errno ));
        exit( 1 );
    }

    last_sa = new sockaddr_in;
}

Config_fd :: ~Config_fd( void )
{
    close( fd );
    delete last_sa;
}

bool
Config_fd :: read( fd_mgr * mgr )
{
    char buf[ max_packet ];
    socklen_t  slen = sizeof( *last_sa );
    int cc;
    cc = recvfrom( fd, buf, max_packet, /* flags */ 0, 
                   (struct sockaddr *) last_sa, & slen );
    printf( "got udp packet of size %d\n", cc );
}

bool
Config_fd :: write( fd_mgr * mgr )
{
    printf( "error this should not be called! \n" );
}

bool
Config_fd :: select_for_read( fd_mgr * mgr )
{
    return true;
}

bool
Config_fd :: select_for_write( fd_mgr * mgr )
{
    return false;
}

bool
Config_fd :: over_write_threshold( void )
{
    return false;
}

bool
Config_fd :: write_to_fd( char * buf, int len )
{
    int cc;
    cc = sendto( fd, buf, len, /* flags */ 0, 
                 (struct sockaddr *) last_sa, sizeof(*last_sa) );
    printf( "wrote udp frame of size %d\n", cc );
}
