
#include "adm_gate.H"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#if defined(SUNOS) || defined(SOLARIS)
#define socklen_t int
#define setsockoptcast char*
#else
#define setsockoptcast void*
#endif

Adm_Gate_fd :: Adm_Gate_fd( int _fd, bool _connecting,
                            bool _doread, bool _dowrite,
                            Adm_pkt_encoder_io * _encoder,
                            Adm_pkt_decoder_io * _decoder )
    : write_buf( max_write )
{
    fd         = _fd;
    doread     = _doread;
    dowrite    = _dowrite;
    connecting = _connecting;

    encode_io = _encoder;
    if ( encode_io )
    {
        encode_io->setup_me( this );
        encoder = new packet_encoder( encode_io );
    }
    else
        encoder = NULL;

    decode_io = _decoder;
    if ( decode_io )
    {
        decode_io->setup_me( this );
        decoder = new packet_decoder( decode_io );
    }
    else
        decoder   = NULL;
}

Adm_Gate_fd :: ~Adm_Gate_fd( void )
{
    close( fd );
    if ( encoder )
        delete encoder;
    if ( encode_io )
        delete encode_io;
    if ( decoder )
        delete decoder;
    if ( decode_io )
        delete decode_io;
}

void
Adm_Gate_fd :: setup_other( Adm_Gate_fd * _other )
{
    other_fd = _other;
    if ( decode_io )
        decode_io->setup_other( other_fd );
}

//virtual
fd_interface::rw_response
Adm_Gate_fd :: read( fd_mgr * )
{
    // since the write threshold is 1/2 of max_write,
    // then 1/3 is safe for this.

    char    read_buf [ max_write / 3 ];
    int     cc;

    if ( connecting )
    {
        printf( "connection failed\n" );
        connecting = false;
        do_close = true;
        other_fd->do_close = true;
        return DEL;
    }

    cc = ::read( fd, read_buf, sizeof( read_buf ));

    if ( cc <= 0 )
    {
        do_close = true;
        other_fd->do_close = true;
        return DEL;
    }

    if ( decoder )
    {
        // pass all data to the packet decoder.
        // if it isn't packet data, the decoder will pass it
        // on to our companion fd.

        decoder->input_bytes( read_buf, cc );
    }
    else
    {
        // we are not a decoder instance. just pass the
        // data thru.

        if ( other_fd->write_to_fd( read_buf, cc ) == false )
        {
            printf( "error writing pkt to other fd\n" );
        }
    }

    return OK;
}

//virtual
fd_interface::rw_response
Adm_Gate_fd :: write( fd_mgr * )
{
    if ( connecting )
    {
        printf( "connection succeeded\n" );
        connecting = false;
        return OK;
    }

    int cc;
    cc = ::write( fd, write_buf.read_pos(), write_buf.contig_readable() );

    if ( cc <= 0 )
    {
        do_close = true;
        other_fd->do_close = true;
        return DEL;
    }

    write_buf.record_read( cc );
    return OK;
}

//virtual
void
Adm_Gate_fd :: select_rw ( fd_mgr *, bool * rd, bool * wr )
{
    if ( !doread )
        *rd = false;
    else if ( other_fd->over_write_threshold() )
        *rd = false;
    else
        //xxx : will need some kind of congestion control
        //      once there are active proxy fds to consider.
        *rd = true;

    if ( !dowrite )
        *wr = false;
    else if ( connecting )
        *wr = true;
    else if ( write_buf.used_space() == 0 )
        *wr = false;
    else
        *wr = true;
}

//virtual
bool
Adm_Gate_fd :: over_write_threshold( void )
{
    if ( write_buf.used_space() > (max_write / 2))
        return true;
    return false;
}

//virtual
bool
Adm_Gate_fd :: write_to_fd( char * buf, int len )
{
    int cc;

    cc = write_buf.write( buf, len );
    if ( cc != len )
    {
        printf( "write_to_fd failed\n" );
        return false;
    }

    return true;
}
