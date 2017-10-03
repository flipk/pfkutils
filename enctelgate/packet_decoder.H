
#ifndef __PACKET_DECODER_H__
#define __PACKET_DECODER_H__

#include "packet.H"

class packet_decoder_io {
public:
    virtual ~packet_decoder_io( void ) { /* nothing */ }
    virtual void outbytes( unsigned char *, int ) = 0;
    virtual void outpacket( unsigned char *, int ) = 0;
};

class packet_decoder {
    packet_decoder_io * io;
    packet_encrypt_decrypt_io * decrypter;

    enum { STATE_HDR_HUNT,
           STATE_GET_LENGTH,
           STATE_GET_BODY,
           STATE_GET_CKSUM } state;
    int substate;

    unsigned char  input_packet[ PACKET_ENCODED_MAX_LENGTH ];
    short  input_packet_length;
    short  input_left;
    unsigned short input_checksum;
    unsigned short calculated_checksum;

    int    b64_in;
    unsigned char  b64_in_4[4];

    int    first_char_after_packet;

    unsigned char  unput_buffer[ PACKET_MAX_B64_DATA ];
    int    unput_in;
    void   unput( unsigned char c );
    void   unput_flush( void );
    void   unput_discard( void ) { unput_in = 0; }

    static const int output_max = 1000;
    unsigned char  output_buffer[ output_max ];
    int    output_pos;
    void   add_output( unsigned char * c, int len );
    void   flush_output( void );

    void   input_byte( unsigned char c );
    void   input_decoded_byte( unsigned char c );

public:
    packet_decoder( packet_decoder_io * _io,
                    packet_encrypt_decrypt_io * _decrypter );
    void input_bytes( unsigned char * buffer, int len ) {
        while ( len-- > 0 )
            input_byte( *buffer++ );
        flush_output();
    }
};

#endif /* __PACKET_DECODER_H__ */
