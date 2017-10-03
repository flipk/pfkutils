
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

    last_seq_no = 0;
    last_response_pkt = NULL;
    last_response_len = 0;
}

Config_fd :: ~Config_fd( void )
{
    close( fd );
    delete last_sa;
}

void
Config_fd :: send_response( void )
{
    sendto( fd, last_response_pkt, last_response_len, 0,
            (struct sockaddr *) last_sa, sizeof( *last_sa ));
}

void
Config_fd :: fill_response( char * buf, int len )
{
    if ( last_response_pkt )
        delete[] last_response_pkt;
    last_response_pkt = new char[len];
    last_response_len = len;
    memcpy( last_response_pkt, buf, len );
}

bool
Config_fd :: check_last_seqno( UINT32 seq )
{
    if ( last_seq_no != seq )
    {
        last_seq_no = seq;
        return true;
    }
    return false;
}

//virtual
fd_interface::rw_response
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
        return OK;
    }
    if ( ! u.gen->verif_checksum() )
    {
        printf( "bogus checksum\n" );
        return OK;
    }

    switch ( u.gen->get_type() )
    {
    case Config_Add_Listen_Port::TYPE:
    {
        printf( "add-listen-port %d\n", u.calp->port_number.get() );
        if ( check_last_seqno( u.calp->sequence_number.get() ) == true )
        {
            //xxx handle
            Config_Add_Listen_Port_Reply  calpr;
            calpr.sequence_number.set( u.calp->sequence_number.get() );
            calpr.set_checksum();
            fill_response( calpr.get_ptr(), calpr.get_len() );
        }
        send_response();
        break;
    }
    case Config_Delete_Listen_Port::TYPE:
    {
        printf( "delete-listen-port %d\n", u.cdlp->port_number.get() );
        if ( check_last_seqno( u.cdlp->sequence_number.get() ) == true )
        {
            //xxx handle
            Config_Delete_Listen_Port_Reply cdlpr;
            cdlpr.sequence_number.set( u.cdlp->sequence_number.get() );
            cdlpr.set_checksum();
            fill_response( cdlpr.get_ptr(), cdlpr.get_len() );
        }
        send_response();
        break;
    }
    case Config_Display_Listen_Ports::TYPE:
    {
        printf( "displaying listen ports:\n" );
        //xxx handle
        break;
    }
    case Config_Display_Proxy_Ports::TYPE:
    {
        printf( "displaying proxy ports:\n" );
        //xxx handle
        break;
    }
    }

    return OK;
}

//virtual
fd_interface::rw_response
Config_fd :: write( fd_mgr * mgr )
{
    printf( "error this should not be called! \n" );
    return DEL;
}

//virtual
void
Config_fd :: select_rw ( fd_mgr *, bool * rd, bool * wr )
{
    *rd = true;
    *wr = false;
}

//virtual
bool
Config_fd :: write_to_fd( char * buf, int len )
{
    int cc;
    cc = sendto( fd, buf, len, /* flags */ 0, 
                 (struct sockaddr *) last_sa, sizeof(*last_sa) );
}
