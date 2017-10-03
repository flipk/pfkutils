
#include "packet_encoder.H"
#include "packet_decoder.H"
#include "fd_mgr.H"
#include "adm_hookup.H"
#include "adm_gate.H"
#include "pipe_mgr.H"
#include "config_fd.H"

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define TEST 1

#if TEST==1
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
    Config_fd                config_fd( &pipe_mgr, 2100 );

    fd_interface * fd1 = new Adm_Hookup_fd( &factory, 2500, "127.1", 2501 );
    fd_interface * fd2 = new Adm_Hookup_fd( &factory, 2502, "127.1", 2503 );

    mgr.register_fd( &config_fd );
    mgr.register_fd( fd1 );
    mgr.register_fd( fd2 );

    mgr.loop();

    return 0;
}
#endif



#if (TEST==2) || (TEST==3)
PkTcpMsgDef( phil_test_msg, 0xcf220fb8,
             INT32_t junk;
    );
#endif


#if TEST==2
class phil_io : public packet_encoder_io {
public:
    void outbytes( char * buf, int len ) {
        ::write( 1, buf, len );
    }
};

int
main()
{
    phil_test_msg   tm;

    tm.junk.set( 12345678 );
    tm.set_checksum();

    int l = tm.get_len();
    char * p = tm.get_ptr();

    phil_io io;
    packet_encoder e( &io );

    e.encode_packet( 4, p, l );
}
#endif


#if TEST==3
class phil_io : public packet_decoder_io {
public:
    void outbytes( char * buf, int l ) {
        printf( "outbytes %d\n", l );
    }
    void outpacket( short pipeno, char * pkt, int pktlen ) {
        printf( "packet on pipe %d\n", pipeno );
        MaxPkMsgType * mpmt = (MaxPkMsgType *) pkt;
        phil_test_msg * ptm;
        if ( mpmt->verif_magic() == false )
            printf( "bogus tcp msg magic\n" );
        else if ( mpmt->verif_checksum() == false )
            printf( "bogus tcp msg cksum\n" );
        else if ( mpmt->convert( &ptm ) == false )
            printf( "not valid phil_test_msg packet!\n" );
        else
            printf( "phil_test_msg junk = %d\n", ptm->junk.get() );
    }
};

#define MSG "PFK1ABAABFBISUwQALgPm1QECAQAAAADAw==\n"

int
main()
{
    phil_io         io;
    packet_decoder  d( &io );

    while ( 1 )
    {
        char buf[ 100 ];
        int cc;
        cc = ::read( 0, buf, sizeof(buf) );
        if ( cc <= 0 )
            break;
        d.input_bytes( buf, cc );
    }
}
#endif
