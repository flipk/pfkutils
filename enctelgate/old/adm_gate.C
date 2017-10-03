
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

class Adm_pkt_decoder_io : public packet_decoder_io {
public:
    Adm_Gate_fd * me;
    Adm_Gate_fd * other;
    Pipe_Mgr    * pipe_mgr;
    Adm_pkt_decoder_io( Adm_Gate_fd * _me, Pipe_Mgr * _pipe_mgr ) {
        me = _me;  pipe_mgr = _pipe_mgr;
    }
    void setup_other ( Adm_Gate_fd * _other ) { other = _other; }
    void outbytes( char *, int );
    void outpacket( short pipeno, char *, int );
};

class Adm_pkt_encoder_io : public packet_encoder_io {
public:
    Adm_Gate_fd * me;
    Adm_pkt_encoder_io( Adm_Gate_fd * _me ) {
        me = _me;
    }
    void outbytes( char *, int );
};

void
Adm_pkt_decoder_io :: outbytes( char *, int )
{
    //xxx
}

void
Adm_pkt_decoder_io :: outpacket( short pipeno, char *, int )
{
    //xxx
}

void
Adm_pkt_encoder_io :: outbytes( char *, int )
{
    //xxx
}

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

    read_amt = 0;
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
    if ( connecting )
    {
        printf( "connection failed\n" );
        connecting = false;
        do_close = true;
        other_fd->do_close = true;
        return false;
    }

    int cc;
    cc = ::read( fd, read_buf, max_write );

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

        read_amt = cc;
        cc = other_fd->write_buf.write( read_buf, read_amt );

        if ( cc != read_amt )
            memmove( read_buf, read_buf + cc, read_amt - cc );
        read_amt -= cc;

        return true;
    }
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

void
Adm_Gate_fd :: more_forward_data( void )
{
    if ( read_amt != 0 )
    {
        int cc;
        //xxx should use new write_to_fd ?
        cc = other_fd->write_buf.write( read_buf, read_amt );
        if ( cc != read_amt )
            memmove( read_buf, read_buf + cc, read_amt - cc );
        read_amt -= cc;
    }
}

bool
Adm_Gate_fd :: select_for_read( fd_mgr * fdmgr )
{
    if ( !doread )
        return false;
    if ( read_amt > 0 )
        return false;
    if ( other_fd->write_buf.free_space() > 0 )
        return true;
    return false;
}

bool
Adm_Gate_fd :: select_for_write( fd_mgr * fdmgr )
{
    if ( !dowrite )
        return false;
    if ( connecting )
        return true;
    if ( write_buf.used_space() == 0 )
        other_fd->more_forward_data();
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
    //xxx
}
