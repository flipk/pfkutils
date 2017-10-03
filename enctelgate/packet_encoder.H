
#ifndef __PACKET_ENCODER_H__
#define __PACKET_ENCODER_H__

#include "packet.H"

class packet_encoder_io {
public:
    virtual ~packet_encoder_io( void ) { /* placeholder */ }
    virtual void outbytes( uchar *, int ) = 0;
};

class packet_encoder_encrypter_io {
public:
    virtual ~packet_encoder_encrypter_io( void ) { /* placeholder */ }
    virtual void encrypt_packet( uchar * in,  int in_len,
                                 uchar * out, int * out_len ) = 0;
};

class packet_encoder {
    packet_encoder_io * io;
    packet_encoder_encrypter_io * encrypter;

    int   encoded_len;
    uchar encoded_packet[ PACKET_ENCODED_MAX_LENGTH ];

    int   b64_in;
    uchar b64_in_chars[3];

    void start_packet( void );
    void add_byte( uchar c );
    void end_packet( void );

    void add_bytes( uchar * c, int len ) {
        while ( len-- > 0 )
            add_byte( *c++ );
    }

public:
    packet_encoder( packet_encoder_io * _io,
                    packet_encoder_encrypter_io * _encrypter );
    void encode_packet( uchar *, int );
};

#endif /* __PACKET_ENCODER_H__ */
