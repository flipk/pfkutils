
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

class Telgate_pkt_encoder_io : public Adm_pkt_encoder_io {
public:
    Telgate_pkt_encoder_io( void ) { }
    /*virtual*/ void outbytes ( char * buf, int len ) {
        if ( me->write_to_fd( buf, len ) == false )
            printf( "Adm_pkt_encoder_io outbytes failure\n" );
    }
};

class Telgate_pkt_decoder_io : public Adm_pkt_decoder_io {
    fd_interface * tun_fdi;
public:
    Telgate_pkt_decoder_io( fd_interface * _tun_fdi ) { 
        tun_fdi = _tun_fdi;
    }
    /*virtual*/ void outbytes ( char * buf, int len ) {
        if ( other->write_to_fd( buf, len ) == false )
            printf( "Adm_pkt_decoder_io outybtes failed\n" );
    }
    /*virtual*/ void outpacket( char *, int ) {
        /* xxx tun_fdi ? */
    }
};

class Adm_User_Hookup_Factory : public Adm_Hookup_Factory_iface {
    fd_mgr * mgr;
    fd_interface * tun_fdi;
public:
    Adm_User_Hookup_Factory( fd_interface * _tunfdi ) { tun_fdi = _tunfdi; }
    ~Adm_User_Hookup_Factory( void ) { /* xxx */ }

    /*virtual*/ void new_gateway( int fd_ear, int fd_outfd, fd_mgr * fdmgr )
    {

        // this one is reading and writing the 'telnet' from the user
        // which gets the connection to the worker started.

        Adm_Gate_fd * gfd1
            = new Adm_Gate_fd( fd_ear,
                               false,            // connecting
                               true, true,       // doread / dowrite
                               NULL, NULL );     // encoder / decoder

        // this one is reading and writing the outgoing
        // network interface to the worker.

        Adm_pkt_decoder_io *  decoder;
        Adm_pkt_encoder_io *  encoder;

        encoder = new Telgate_pkt_encoder_io;
        decoder = new Telgate_pkt_decoder_io( tun_fdi );

        Adm_Gate_fd * gfd2
            = new Adm_Gate_fd( fd_outfd,
                               true,             // connecting
                               true, true,       // doread / dowrite
                               encoder, decoder  );

        gfd1->setup_other( gfd2 );
        gfd2->setup_other( gfd1 );

        fdmgr->register_fd( gfd1 );
        fdmgr->register_fd( gfd2 );
    }
};

int
main()
{
    fd_mgr  mgr( /*debug*/ false, /*threshold*/ 0 );
    fd_interface * fdi;

    fdi = new Tunnel( "/dev/tun0", "11.0.0.2", "11.0.0.1" );
    mgr.register_fd( fdi );

    Adm_User_Hookup_Factory  hookup( fdi );

    // this FD waits for an incoming telnet connection;
    // when established, it will form an outgoing telnet
    // connection (eventually) going to the worker host.

    fdi = new Adm_Hookup_fd( &hookup, 2700,
                             // the following line to be replaced
                             // with the worker host
                             "127.1", 23 );
    mgr.register_fd( fdi );

    mgr.loop();

    return 0;
}
