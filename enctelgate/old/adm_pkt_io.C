
#include "fd_mgr.H"
#include "pipe_mgr.H"
#include "adm_gate.H"
#include "adm_pkt_io.H"

void
Adm_pkt_decoder_io :: outbytes( char * buf, int len )
{
    if ( other->write_to_fd( buf, len ) == false )
        printf( "Adm_pkt_decoder_io outybtes failed\n" );
}

void
Adm_pkt_decoder_io :: outpacket( short pipeno, char * buf, int len )
{
    pipe_mgr->handle_pkt( pipeno, buf, len );
}

void
Adm_pkt_encoder_io :: outbytes( char * buf, int len )
{
    if ( me->write_to_fd( buf, len ) == false )
        printf( "Adm_pkt_encoder_io outbytes failure\n" );
}
