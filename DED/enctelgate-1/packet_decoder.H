
#ifndef __PACKET_DECODER_H__
#define __PACKET_DECODER_H__

#include "packet.H"


class packet_decoder_io {
public:
    virtual ~packet_decoder_io( void ) { /* nothing */ }
    virtual void outbytes( char *, int ) = 0;
    virtual void outpacket( short pipeno, char *, int ) = 0;
};

class packet_decoder {
    packet_decoder_io * io;

    enum { STATE_HDR_HUNT,
           STATE_GET_LENGTH,
           STATE_GET_PIPENO,
           STATE_GET_BODY,
           STATE_GET_CKSUM } state;
    int substate;

    char   input_packet[ PACKET_ENCODED_MAX_LENGTH ];
    short  input_packet_length;
    short  input_pipeno;
    short  input_left;
    ushort input_checksum;
    ushort calculated_checksum;

    int   b64_in;
    char  b64_in_4[4];

    bool   first_char_after_packet;

    char  unput_buffer[ PACKET_MAX_B64_DATA ];
    int   unput_in;
    void   unput( char c );
    void   unput_flush( void );
    void   unput_discard( void ) { unput_in = 0; }

    static const int output_max = 1000;
    char  output_buffer[ output_max ];
    int   output_pos;
    void   add_output( char * c, int len );
    void   flush_output( void );

    void   input_byte( char c );
    void   input_decoded_byte( char c );

public:
    packet_decoder( packet_decoder_io * _io );
    void input_bytes( char * buffer, int len ) {
        while ( len-- > 0 )
            input_byte( *buffer++ );
        flush_output();
    }
};

#endif /* __PACKET_DECODER_H__ */
