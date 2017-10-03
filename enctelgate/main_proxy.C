
#include "packet_encoder.H"
#include "packet_decoder.H"
#include "fd_mgr.H"
#include "adm_gate.H"
#include "adm_hookup.H"
#include "tunnel.H"
#include "d3encdec.H"

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "sudo.h"

class Telgate_pkt_decoder_io : public Adm_pkt_decoder_io {
    Tunnel_fd * tun_fdi;
public:
    Telgate_pkt_decoder_io( Tunnel_fd * _tun_fdi ) { 
        tun_fdi = _tun_fdi;
    }
    ~Telgate_pkt_decoder_io( void ) {
        tun_fdi->unregister_encoder_fd();
    }
    /*virtual*/ void outpacket( unsigned char * buf, int len ) {
        if ( tun_fdi->write_to_fd( (char*)buf, len ) == false )
            printf( "failed to write to tun fd?\n" );
    }
};

class Adm_User_Hookup_Factory : public Adm_Hookup_Factory_iface {
    fd_mgr * mgr;
    Tunnel_fd * tun_fdi;
    d3des_crypt * crypt;
public:
    Adm_User_Hookup_Factory( Tunnel_fd * _tunfdi,
                             d3des_crypt * _crypt ) {
        tun_fdi = _tunfdi; crypt = _crypt; }
    /*virtual*/ ~Adm_User_Hookup_Factory( void ) { delete tun_fdi; }
    /*virtual*/ void new_gateway( int fd_ear, int fd_outfd, fd_mgr * fdmgr )
    {
        // this one is reading and writing the 'telnet' from the user
        // which gets the connection to the worker started.

        Adm_Gate_fd * gfd1
            = new Adm_Gate_fd( fd_ear,
                               false,            // connecting
                               true, true,       // doread / dowrite
                               false,            // doencode
                               NULL,             // encrypt/decrypter
                               NULL);            // decoder

        // this one is reading and writing the outgoing
        // network interface to the worker.

        Adm_pkt_decoder_io *  decoder
            = new Telgate_pkt_decoder_io( tun_fdi );

        Adm_Gate_fd * gfd2
            = new Adm_Gate_fd( fd_outfd,
                               true,             // connecting
                               true, true,       // doread / dowrite
                               true,             // doencode
                               crypt,            // encrypt/decrypter
                               decoder );        // decoder

        tun_fdi->register_encoder_fd( gfd2 );
        gfd1->setup_other( gfd2 );
        gfd2->setup_other( gfd1 );

        fdmgr->register_fd( gfd1 );
        fdmgr->register_fd( gfd2 );
    }
};

extern "C" int
etg_proxy_main( int argc, char ** argv )
{
    if ( argc != 4 )
    {
        fprintf( stderr,
                 "usage:\n"
                 "   etg_proxy <proxy_port> <worker_host> <worker_port>\n"
                 "\n" );
        exit( 1 );
    }

    char *       my_ip = (char*) "11.0.0.1";
    char *    other_ip = (char*) "11.0.0.2";
    char *     netmask = (char*) "255.255.255.0";
    short   proxy_port = atoi( argv[1] );
    char * worker_host =       argv[2];
    short  worker_port = atoi( argv[3] );

    fd_mgr  mgr( /*debug*/ false, /*threshold*/ 0 );
    Tunnel_fd * tun_fdi;
    fd_interface * fdi;

    tun_fdi = new Tunnel_fd( my_ip, other_ip, netmask );
    mgr.register_fd( tun_fdi );

    // this FD waits for an incoming telnet connection;
    // when established, it will form an outgoing telnet
    // connection (eventually) going to the worker host.

    d3des_crypt   * crypt  = d3des_crypt_loadkey();

    Adm_User_Hookup_Factory  hookup( tun_fdi, crypt );
    fdi = new Adm_Hookup_fd( &hookup, proxy_port, worker_host, worker_port );
    mgr.register_fd( fdi );

    // revoke our priviledges because we don't
    // need them anymore.
    setreuid(MY_UID,MY_UID);
    setregid(MY_GID,MY_GID);

    mgr.loop();

    return 0;
}
