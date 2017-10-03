
#include <stdio.h>
#include <string.h>
#include "packet_decoder.H"
#include "base64.h"

packet_decoder :: packet_decoder( packet_decoder_io * _io,
                                  packet_encrypt_decrypt_io * _decrypter )
{
    io = _io;
    decrypter = _decrypter;
    state = STATE_HDR_HUNT;
    substate = 0;
    b64_in = 0;
    unput_in = 0;
    output_pos = 0;
    first_char_after_packet = 0;
}

void
packet_decoder :: unput( uchar c )
{
    int new_in;
    new_in = unput_in + 1;
    if ( new_in == PACKET_MAX_B64_DATA )
    {
        printf( "packet_decoder unput buffer full!!\n" );
        return;
    }
    unput_buffer[unput_in] = c;
    unput_in = new_in;
}

void
packet_decoder :: unput_flush( void )
{
    if ( unput_in == 0 )
        return;
    add_output( unput_buffer, unput_in );
    unput_in = 0;
}

void
packet_decoder :: add_output( uchar * c, int len )
{
    while ( len > 0 )
    {
        int tocopy = len;
        if ( tocopy > ( output_max - output_pos ))
            tocopy = output_max - output_pos;
        memcpy( output_buffer + output_pos, c, tocopy );
        output_pos += tocopy;
        len -= tocopy;
        if ( output_pos == output_max )
            flush_output();
    }
}

void
packet_decoder :: flush_output( void )
{
    if ( output_pos > 0 )
        io->outbytes( output_buffer, output_pos );
    output_pos = 0;
}

void
packet_decoder :: input_byte( uchar c )
{
    stats.in_bytes ++;

    if ( first_char_after_packet > 0 )
    {
        if ( c == '\n' )
            first_char_after_packet--;
            /* skip */
            return;
    }

    unput(c);

    if ( state == STATE_HDR_HUNT )
    {
        if ( c != PACKET_HEADER_STRING[substate] )
        {
            substate = 0;
            unput_flush();
            return;
        }
        /* else */

        if ( ++substate == PACKET_HEADER_LENGTH )
        {
            state = STATE_GET_LENGTH;
            substate = 0;
            b64_in = 0;
            input_packet_length = 0;
        }

        return;
    }

    if ( ! b64_is_valid_char( c ))
    {
        if ( c == '\n' || c == '\r' || c == ' ' || c == '\t' )
            /* skip */
            return;
        state = STATE_HDR_HUNT;
        substate = 0;
        unput_flush();
        return;
    }

    b64_in_4[b64_in] = c;
    if ( ++b64_in == 4 )
    {
        int cc;
        uchar b64_out_3[3];
        uchar * cp = b64_out_3;

        cc = b64_decode_quantum( b64_in_4, b64_out_3 );
        while ( cc-- > 0 && state != STATE_HDR_HUNT )
            input_decoded_byte( *cp++ );

        /* all further errors, unputs and state dropouts 
           handled by the above func */

        b64_in = 0;
    }
}

void
packet_decoder :: input_decoded_byte( uchar c )
{
    switch ( state )
    {
    case STATE_GET_LENGTH:
        input_packet_length <<= 8;
        input_packet_length += c;
        if ( ++substate == PACKET_LENGTH_LENGTH )
        {
            if ( input_packet_length > PACKET_MAX_PAYLOAD_DATA )
            {
                /* invalid packet!! */
                state = STATE_HDR_HUNT;
                substate = 0;
                unput_flush();
                return;
            }
            state = STATE_GET_BODY;
            substate = 0;
            input_left = input_packet_length;
            calculated_checksum = 0;
        }
        return;

    case STATE_GET_BODY:
        input_packet[ substate ] = c;
        calculated_checksum += c;
        if ( ++substate == input_packet_length )
        {
            state = STATE_GET_CKSUM;
            substate = 0;
            input_checksum = 0;
        }
        return;

    case STATE_GET_CKSUM:
        input_checksum <<= 8;
        input_checksum += c;
        if ( ++substate == sizeof( input_checksum ))
        {
            state = STATE_HDR_HUNT;
            substate = 0;
            if ( input_checksum != calculated_checksum )
            {
                printf( "checksum error length %d %x != %x\n",
                        input_packet_length,
                        input_checksum, calculated_checksum );
                unput_flush();
            }
            else
            {
                unput_discard();
                if ( decrypter )
                {
                    uchar decbuf[ input_packet_length * 15 / 10 ];
                    int   declen = sizeof( decbuf );
                    bool result =
                        decrypter->decrypt_packet( input_packet,
                                                   input_packet_length,
                                                   decbuf, &declen );
                    if ( result )
                        io->outpacket( decbuf, declen );
                }
                else
                    io->outpacket( input_packet, input_packet_length );
                first_char_after_packet = 1;
                stats.in_packets ++;
                print_stats();
            }
        }
        return;
    }
}
