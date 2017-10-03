
#ifndef __PACKET_DECODER_H__
#define __PACKET_DECODER_H__

#include "packet.H"


class packet_decoder_io {
public:
    virtual ~packet_decoder_io( void ) { /* nothing */ }
    virtual void outbytes( uchar *, int ) = 0;
    virtual void outpacket( uchar *, int ) = 0;
};

class packet_decoder_decrypter_io {
public:
    virtual ~packet_decoder_decrypter_io( void ) { /* placeholder */ }
    virtual bool decrypt_packet( uchar * in,  int in_len,
                                 uchar * out, int * out_len ) = 0;
};

class packet_decoder {
    packet_decoder_io * io;
    packet_decoder_decrypter_io * decrypter;

    enum { STATE_HDR_HUNT,
           STATE_GET_LENGTH,
           STATE_GET_BODY,
           STATE_GET_CKSUM } state;
    int substate;

    uchar  input_packet[ PACKET_ENCODED_MAX_LENGTH ];
    short  input_packet_length;
    short  input_left;
    ushort input_checksum;
    ushort calculated_checksum;

    int    b64_in;
    uchar  b64_in_4[4];

    int    first_char_after_packet;

    uchar  unput_buffer[ PACKET_MAX_B64_DATA ];
    int    unput_in;
    void   unput( uchar c );
    void   unput_flush( void );
    void   unput_discard( void ) { unput_in = 0; }

    static const int output_max = 1000;
    uchar  output_buffer[ output_max ];
    int    output_pos;
    void   add_output( uchar * c, int len );
    void   flush_output( void );

    void   input_byte( uchar c );
    void   input_decoded_byte( uchar c );

public:
    packet_decoder( packet_decoder_io * _io,
                    packet_decoder_decrypter_io * _decrypter );
    void input_bytes( uchar * buffer, int len ) {
        while ( len-- > 0 )
            input_byte( *buffer++ );
        flush_output();
    }
};

#endif /* __PACKET_DECODER_H__ */
