
#ifndef __PACKET_ENCODER_H__
#define __PACKET_ENCODER_H__

#include "packet.H"

class packet_encoder_io {
public:
    virtual ~packet_encoder_io( void ) { /* placeholder */ }
    virtual void outbytes( char *, int ) = 0;
};

class packet_encoder {
    packet_encoder_io * io;

    int  encoded_len;
    char encoded_packet[ PACKET_ENCODED_MAX_LENGTH ];

    int  b64_in;
    char b64_in_chars[3];

    void start_packet( void );
    void add_byte( char c );
    void end_packet( void );

    void add_bytes( char * c, int len ) {
        while ( len-- > 0 )
            add_byte( *c++ );
    }

public:
    packet_encoder( packet_encoder_io * _io );
    void encode_packet( short pipeno, char *, int );
};

#endif /* __PACKET_ENCODER_H__ */
