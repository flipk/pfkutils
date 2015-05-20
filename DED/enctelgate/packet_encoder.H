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

#ifndef __PACKET_ENCODER_H__
#define __PACKET_ENCODER_H__

#include "packet.H"

class packet_encoder_io {
public:
    virtual ~packet_encoder_io( void ) { /* placeholder */ }
    virtual void outbytes( unsigned char *, int ) = 0;
};

class packet_encoder {
    packet_encoder_io * io;
    packet_encrypt_decrypt_io * encrypter;

    int   encoded_len;
    unsigned char encoded_packet[ PACKET_ENCODED_MAX_LENGTH ];

    int   b64_in;
    unsigned char b64_in_chars[3];

    void start_packet( void );
    void add_byte( unsigned char c );
    void end_packet( void );

    void add_bytes( unsigned char * c, int len ) {
        while ( len-- > 0 )
            add_byte( *c++ );
    }

public:
    packet_encoder( packet_encoder_io * _io,
                    packet_encrypt_decrypt_io * _encrypter );
    void encode_packet( unsigned char *, int );
};

#endif /* __PACKET_ENCODER_H__ */
