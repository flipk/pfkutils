
#include "packet_encoder.H"
#include "base64.h"

#include <stdio.h>
#include <string.h>

packet_encoder :: packet_encoder( packet_encoder_io * _io,
                                  packet_encrypt_decrypt_io * _encrypter )
{
    io = _io;
    encrypter = _encrypter;
}

void
packet_encoder :: start_packet( void )
{
    memcpy( encoded_packet, PACKET_HEADER_STRING, PACKET_HEADER_LENGTH );
    encoded_len = PACKET_HEADER_LENGTH;
    b64_in = 0;
}

void
packet_encoder :: add_byte( uchar c )
{
    int linectr = PACKET_HEADER_LENGTH + PACKET_LENGTH_LENGTH;
    b64_in_chars[ b64_in++ ] = c;
    if ( b64_in == 3 )
    {
        int enc;
        enc = b64_encode_quantum( b64_in_chars, 3,
                                  encoded_packet + encoded_len );
        encoded_len += enc;
        linectr     += enc;
        if ( linectr >= NUM_CHARS_BETWEEN_NEWLINES )
        {
            encoded_packet[ encoded_len++ ] = '\n';
            linectr = 0;
            stats.out_bytes ++;
        }
        b64_in = 0;
    }
    stats.out_bytes ++;
}

void
packet_encoder :: end_packet( void )
{
    int enc;
    if ( b64_in != 0 )
    {
        enc = b64_encode_quantum( b64_in_chars, b64_in,
                                  encoded_packet + encoded_len );
        encoded_len += enc;
        b64_in = 0;
    }
    encoded_packet[ encoded_len++ ] = '\n';
    io->outbytes( encoded_packet, encoded_len );
    stats.out_packets ++;
    print_stats();
}

void
packet_encoder :: encode_packet( uchar *pkt, int pkt_len )
{
    if ( pkt_len > PACKET_MAX_PAYLOAD_DATA )
    {
        printf( "encoding packet too big!!\n" );
        return;
    }

    start_packet();
    short calculated_checksum = 0;
    int i;

    if ( encrypter )
    {
        uchar encbuf[ pkt_len * 15 / 10 ];
        int   enclen = sizeof( encbuf );

        encrypter->encrypt_packet( pkt, pkt_len, encbuf, &enclen );

        add_byte( (enclen >> 8) & 0xff );
        add_byte( (enclen     ) & 0xff );

        add_bytes( encbuf, enclen );

        for ( i = 0; i < enclen; i++ )
            calculated_checksum += encbuf[i];
    }
    else
    {
        add_byte( (pkt_len >> 8) & 0xff );
        add_byte( (pkt_len     ) & 0xff );

        add_bytes( pkt, pkt_len );

        for ( i = 0; i < pkt_len; i++ )
            calculated_checksum += pkt[i];
    }

    add_byte( (calculated_checksum >> 8) & 0xff );
    add_byte( (calculated_checksum     ) & 0xff );

    end_packet();
}
