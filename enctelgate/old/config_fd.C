
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

    seq_no_pos = 0;
    memset( &seq_nos, 0, sizeof( seq_nos ));
}

Config_fd :: ~Config_fd( void )
{
    close( fd );
    delete last_sa;
}

bool
Config_fd :: read( fd_mgr * mgr )
{
    char buf[ LARGEST_CONFIG_MSG_SIZE ];
    union {
        char                         * buf;
        MaxPkMsgType                 * gen;
        Config_Add_Listen_Port       * calp;
        Config_Delete_Listen_Port    * cdlp;
        Config_Display_Listen_Ports  * cdisplp;
        Config_Display_Proxy_Ports   * cdisppp;
    } u;

    socklen_t  slen = sizeof( *last_sa );
    int cc;
    u.buf = buf;
    cc = recvfrom( fd, buf, sizeof(buf),
                   /* flags */ 0, 
                   (struct sockaddr *) last_sa, & slen );

    if ( ! u.gen->verif_magic() )
    {
        printf( "bogus magic\n" );
        return true;
    }
    if ( ! u.gen->verif_checksum() )
    {
        printf( "bogus checksum\n" );
        return true;
    }

    switch ( u.gen->get_type() )
    {
    case Config_Add_Listen_Port::TYPE:
    {
        printf( "add-listen-port %d\n", u.calp->port_number.get() );
        //xxx handle it
        Config_Add_Listen_Port_Reply  calpr;
        calpr.sequence_number.set( u.calp->sequence_number.get() );
        calpr.set_checksum();
        sendto( fd, calpr.get_ptr(), calpr.get_len(), 0,
                (struct sockaddr *)last_sa, sizeof( *last_sa ));
        break;
    }
    case Config_Delete_Listen_Port::TYPE:
    {
        printf( "delete-listen-port %d\n", u.cdlp->port_number.get() );
        //xxx
        Config_Delete_Listen_Port_Reply cdlpr;
        cdlpr.sequence_number.set( u.cdlp->sequence_number.get() );
        cdlpr.set_checksum();
        sendto( fd, cdlpr.get_ptr(), cdlpr.get_len(), 0,
                (struct sockaddr *)last_sa, sizeof( *last_sa ));
        break;
    }
    case Config_Display_Listen_Ports::TYPE:
    {
        printf( "displaying listen ports:\n" );
        //xxx
        break;
    }
    case Config_Display_Proxy_Ports::TYPE:
    {
        printf( "displaying proxy ports:\n" );
        //xxx
        break;
    }
    }

    return true;
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
