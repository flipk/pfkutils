/*
    This file is part of the "pfkutils" tools written by Phil Knaack
    (pknaack1@netscape.net).
    Copyright (C) 2008  Phillip F Knaack

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

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

class Adm_pkt_encoder_io : public packet_encoder_io {
protected:
    Adm_Gate_fd  * me;
public:
    Adm_pkt_encoder_io ( void ) { /* nothing */ }
    void setup_me      ( Adm_Gate_fd * _me    ) { me = _me; }
    /*virtual*/ void outbytes ( unsigned char * buf, int len ) {
        if ( me->write_to_fd( (char*)buf, len ) == false )
            printf( "Adm_pkt_encoder_io outbytes failure\n" );
    }
};

//virtual
void
Adm_pkt_decoder_io :: outbytes ( unsigned char * buf, int len )
{
    if ( other->write_to_fd( (char*)buf, len ) == false )
        printf( "Adm_pkt_decoder_io outybtes failed\n" );
}

Adm_Gate_fd :: Adm_Gate_fd( int _fd, bool _connecting,
                            bool _doread, bool _dowrite,
                            bool _doencode,
                            packet_encrypt_decrypt_io * _enc_dec,
                            Adm_pkt_decoder_io * _decoder )
    : write_buf( max_write )
{
    fd         = _fd;
    doread     = _doread;
    dowrite    = _dowrite;
    connecting = _connecting;

    if ( _doencode )
        encode_io = new Adm_pkt_encoder_io;
    else
        encode_io = NULL;

    if ( encode_io )
    {
        encode_io->setup_me( this );
        encoder = new packet_encoder( encode_io, _enc_dec );
    }
    else
        encoder = NULL;

    decode_io = _decoder;
    if ( decode_io )
    {
        decode_io->setup_me( this );
        decoder = new packet_decoder( decode_io, _enc_dec );
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

    unsigned char   read_buf [ max_write / 3 ];
    int     cc;

    cc = ::read( fd, read_buf, sizeof( read_buf ));

    if ( connecting && cc <= 0 )
    {
        fprintf( stderr, "connection failed: %s\n", strerror( errno ));
        connecting = false;
        do_close = true;
        other_fd->do_close = true;
        return DEL;
    }

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

        if ( other_fd->write_to_fd( (char*)read_buf, cc ) == false )
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
        connecting = false;
        return OK;
    }

    int wr = write_buf.contig_readable();
    int cc;
    cc = ::write( fd, write_buf.read_pos(), wr );

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
        //xxx : will need some kind of congestion control ?
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

bool
Adm_Gate_fd :: write_packet_to_fd( unsigned char * buf, int len )
{
    encoder->encode_packet( buf, len );
}
