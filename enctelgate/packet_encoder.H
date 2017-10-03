/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
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
