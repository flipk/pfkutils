
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
    /*virtual*/ ~Telgate_pkt_decoder_io( void ) {
        tun_fdi->unregister_encoder_fd();
    }
    /*virtual*/ void outpacket( unsigned char * buf, int len ) {
        if ( tun_fdi->write_to_fd( (char*)buf, len ) == false )
            printf( "failed to write to tun fd?\n" );
    }
};

extern "C" int
etg_worker_main( int argc, char ** argv )
{
    char *    my_ip = (char*) "11.0.0.2";
    char * other_ip = (char*) "11.0.0.1";
    char *  netmask = (char*) "255.255.255.0";

    fd_mgr  mgr( /*debug*/ false, /*threshold*/ 1 );
    Tunnel_fd * tun_fdi;
    fd_interface * fdi;

    system( "stty -echo" );
    tun_fdi = new Tunnel_fd( my_ip, other_ip, netmask );

    Adm_pkt_decoder_io * decoder
        = new Telgate_pkt_decoder_io( tun_fdi );


    d3des_crypt   * crypt  = d3des_crypt_loadkey();

    // this fd receives input data on fd0 which comes from the proxy
    // and decodes and forwards packets to my tunnel.

    Adm_Gate_fd * gfd1
        = new Adm_Gate_fd( 0,                  // fd
                           false,              // connecting
                           true, false,        // doread / dowrite
                           false,              // doencode
                           crypt,              // encrypt/decrypter
                           decoder );          // decoder

    // this fd takes packets from the tunnel and encodes them, writing
    // them on fd 1 to the proxy.

    Adm_Gate_fd * gfd2
        = new Adm_Gate_fd( 1,               // fd
                           false,           // connecting
                           false, true,     // doread / dowrite
                           true,            // doencode
                           crypt,           // encrypt/decrypter
                           NULL );          // decoder

    tun_fdi->register_encoder_fd( gfd2 );
    gfd1->setup_other( gfd2 );
    gfd2->setup_other( gfd1 );

    mgr.register_fd( tun_fdi );
    mgr.register_fd( gfd1 );
    mgr.register_fd( gfd2 );

    // revoke our priviledges because we don't
    // need them anymore.
    setreuid(MY_UID,MY_UID);
    setregid(MY_GID,MY_GID);

#if 0
//
//  xxxx environment variable to tell etg_worker where to get its key
//
#define FAKEROOT "/home/pknaack1/proj/pfkteld/root"
    if (chdir( FAKEROOT ) < 0)
    {
        printf("chroot unavailable; goodbye\n");
        fprintf(stderr,"chroot unavailable!\n");
        return 2;
    }

    chroot( FAKEROOT );
#endif


    mgr.loop();

    return 0;
}
