
#include "adm_gate.H"
#include "adm_pkt_io.H"

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
                            bool doencoder, bool dodecoder,
                            Pipe_Mgr * _pipemgr )
    : write_buf( max_write )
{
    fd         = _fd;
    pipe_mgr   = _pipemgr;
    doread     = _doread;
    dowrite    = _dowrite;
    connecting = _connecting;

    if ( doencoder )
    {
        encode_io = new Adm_pkt_encoder_io( this );
        encoder   = new packet_encoder( encode_io );
        pipe_mgr->register_encoder( encoder );
    }
    else
    {
        encode_io = NULL;
        encoder   = NULL;
    }

    if ( dodecoder )
    {
        decode_io = new Adm_pkt_decoder_io( this, pipe_mgr );
        decoder   = new packet_decoder( decode_io );
    }
    else
    {
        decode_io = NULL;
        decoder   = NULL;
    }
}

Adm_Gate_fd :: ~Adm_Gate_fd( void )
{
    close( fd );
    if ( encoder )
    {
        delete encoder;
        pipe_mgr->deregister_encoder();
    }
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

bool
Adm_Gate_fd :: read( fd_mgr * fdmgr )
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
        return false;
    }

    cc = ::read( fd, read_buf, sizeof( read_buf ));

    if ( cc <= 0 )
    {
        do_close = true;
        other_fd->do_close = true;
        return false;
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

    return true;
}

bool
Adm_Gate_fd :: write( fd_mgr * fdmgr )
{
    if ( connecting )
    {
        printf( "connection succeeded\n" );
        connecting = false;
        return true;
    }

    int cc;
    cc = ::write( fd, write_buf.read_pos(), write_buf.contig_readable() );

    if ( cc <= 0 )
    {
        do_close = true;
        other_fd->do_close = true;
        return false;
    }

    write_buf.record_read( cc );
    return true;
}

bool
Adm_Gate_fd :: select_for_read( fd_mgr * fdmgr )
{
    if ( !doread )
        return false;
    if ( other_fd->over_write_threshold() )
        return false;

    //xxx : will need some kind of congestion control
    //      once there are active proxy fds to consider.

    return true;
}

bool
Adm_Gate_fd :: select_for_write( fd_mgr * fdmgr )
{
    if ( !dowrite )
        return false;
    if ( connecting )
        return true;
    if ( write_buf.used_space() == 0 )
        return false;
    return true;
}

bool
Adm_Gate_fd :: over_write_threshold( void )
{
    if ( write_buf.used_space() > (max_write / 2))
        return true;
    return false;
}

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
