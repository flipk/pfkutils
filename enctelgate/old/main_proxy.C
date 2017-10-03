
#include "packet_encoder.H"
#include "packet_decoder.H"
#include "fd_mgr.H"
#include "adm_hookup.H"
#include "adm_gate.H"
#include "pipe_mgr.H"
#include "config_fd.H"
#include "config_port.H"

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

class Adm_Gateway_factory : public Adm_Hookup_Factory_iface {
public:
    Pipe_Mgr * pipe_mgr;
    Adm_Gateway_factory( Pipe_Mgr * _pipe_mgr ) {
        pipe_mgr = _pipe_mgr;
    }
    void new_gateway( int fd_ear, int fd_outfd, fd_mgr * fdmgr )
    {
        Adm_Gate_fd * gfd1
            = new Adm_Gate_fd( fd_ear,
                               false,        // connecting
                               true, true,   // doread / dowrite
                               true, true, // doencode / dodecode
                               pipe_mgr );   // pipe mgr

        Adm_Gate_fd * gfd2
            = new Adm_Gate_fd( fd_outfd,
                               true,         // connecting
                               true, true,   // doread / dowrite
                               false, false, // doencode / dodecode
                               pipe_mgr );   // pipe mgr

        gfd1->setup_other( gfd2 );
        gfd2->setup_other( gfd1 );

        fdmgr->register_fd( gfd1 );
        fdmgr->register_fd( gfd2 );
    }
};

int
main()
{
    fd_mgr                   mgr;
    fd_interface           * hkup;
    Pipe_Mgr                 pipe_mgr;
    Adm_Gateway_factory      factory ( &pipe_mgr );
    Config_fd                config_fd( &pipe_mgr, CONFIG_UDP_PORT_NO );

    fd_interface * fd1 = new Adm_Hookup_fd( &factory, 2500, "127.1", 2501 );
    fd_interface * fd2 = new Adm_Hookup_fd( &factory, 2502, "127.1", 2503 );

    mgr.register_fd( &config_fd );
    mgr.register_fd( fd1 );
    mgr.register_fd( fd2 );

    mgr.loop();

    return 0;
}
