
#include "packet_encoder.H"
#include "packet_decoder.H"
#include "fd_mgr.H"
#include "adm_gate.H"
#include "adm_hookup.H"
#include "tunnel.H"

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

class Telgate_pkt_decoder_io : public Adm_pkt_decoder_io {
    Tunnel_fd * tun_fdi;
public:
    Telgate_pkt_decoder_io( Tunnel_fd * _tun_fdi ) { 
        tun_fdi = _tun_fdi;
    }
    ~Telgate_pkt_decoder_io( void ) {
        tun_fdi->unregister_encoder_fd();
    }
    /*virtual*/ void outpacket( uchar * buf, int len ) {
        if ( tun_fdi->write_to_fd( (char*)buf, len ) == false )
            printf( "failed to write to tun fd?\n" );
    }
};

class Adm_User_Hookup_Factory : public Adm_Hookup_Factory_iface {
    fd_mgr * mgr;
    Tunnel_fd * tun_fdi;
public:
    Adm_User_Hookup_Factory( Tunnel_fd * _tunfdi ) { tun_fdi = _tunfdi; }
    ~Adm_User_Hookup_Factory( void ) { delete tun_fdi; }

    /*virtual*/ void new_gateway( int fd_ear, int fd_outfd, fd_mgr * fdmgr )
    {
        // this one is reading and writing the 'telnet' from the user
        // which gets the connection to the worker started.

        Adm_Gate_fd * gfd1
            = new Adm_Gate_fd( fd_ear,
                               false,            // connecting
                               true, true,       // doread / dowrite
                               false, NULL );    // doencode, decoder

        // this one is reading and writing the outgoing
        // network interface to the worker.

        Adm_pkt_decoder_io *  decoder
            = new Telgate_pkt_decoder_io( tun_fdi );

        Adm_Gate_fd * gfd2
            = new Adm_Gate_fd( fd_outfd,
                               true,             // connecting
                               true, true,       // doread / dowrite
                               true, decoder );  // doencode / decoder

        tun_fdi->register_encoder_fd( gfd2 );
        gfd1->setup_other( gfd2 );
        gfd2->setup_other( gfd1 );

        fdmgr->register_fd( gfd1 );
        fdmgr->register_fd( gfd2 );
    }
};

int
main( int argc, char ** argv )
{
    if ( argc != 8 )
    {
        fprintf( stderr,
                 "usage:\n"
                 "   etg_proxy <tun> <my_ip> <other_ip> <netmask> \n"
                 "                 <proxy_port> <worker_host> <worker_port> \n"
                 "\n" );
        exit( 1 );
    }

    int tunnum         = atoi( argv[1] );
    char * my_ip       = argv[2];
    char * other_ip    = argv[3];
    char * netmask     = argv[4];
    short proxy_port   = atoi( argv[5] );
    char * worker_host = argv[6];
    short worker_port  = atoi( argv[7] );

    char tundev[ 40 ];

    sprintf( tundev, "/dev/tun%d", tunnum );

    fd_mgr  mgr( /*debug*/ false, /*threshold*/ 0 );
    Tunnel_fd * tun_fdi;
    fd_interface * fdi;

    tun_fdi = new Tunnel_fd( tundev, my_ip, other_ip, netmask );
    mgr.register_fd( tun_fdi );

    // this FD waits for an incoming telnet connection;
    // when established, it will form an outgoing telnet
    // connection (eventually) going to the worker host.

    Adm_User_Hookup_Factory  hookup( tun_fdi );
    fdi = new Adm_Hookup_fd( &hookup, proxy_port, worker_host, worker_port );
    mgr.register_fd( fdi );

    mgr.loop();

    return 0;
}
