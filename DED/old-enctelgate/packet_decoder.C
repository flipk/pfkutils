
#include <stdio.h>
#include <string.h>
#include "packet_decoder.H"
#include "base64.h"

packet_decoder :: packet_decoder( packet_decoder_io * _io )
{
    io = _io;
    state = STATE_HDR_HUNT;
    substate = 0;
    b64_in = 0;
    unput_in = 0;
    output_pos = 0;
    first_char_after_packet = false;
}

void
packet_decoder :: unput( char c )
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
packet_decoder :: add_output( char * c, int len )
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
packet_decoder :: input_byte( char c )
{
    if ( first_char_after_packet  &&
         ( c == '\n' || c == '\r' ))
    {
        first_char_after_packet = false;
        /* skip */
        return;
    }

    first_char_after_packet = false;
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
            printf( "switch to STATE_GET_LENGTH\n" );
            substate = 0;
            b64_in = 0;
            input_packet_length = 0;
        }

        return;
    }

    if ( ! b64_is_valid_char( c ))
    {
        if ( c == '\n' || c == '\r' || c == ' ' || c == '\t' )
            return;
        state = STATE_HDR_HUNT;
        printf( "switch to STATE_HDR_HUNT\n" );
        substate = 0;
        unput_flush();
        return;
    }

    b64_in_4[b64_in] = c;
    if ( ++b64_in == 4 )
    {
        int cc;
        char b64_out_3[3];
        char * cp = b64_out_3;

        cc = b64_decode_quantum( b64_in_4, b64_out_3 );
        while ( cc-- > 0 && state != STATE_HDR_HUNT )
            input_decoded_byte( *cp++ );

        /* all further errors, unputs and state dropouts 
           handled by the above func */

        b64_in = 0;
    }
}

void
packet_decoder :: input_decoded_byte( char c )
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
                printf( "switch to STATE_HDR_HUNT\n" );
                substate = 0;
                unput_flush();
                return;
            }
            state = STATE_GET_PIPENO;
            printf( "switch to STATE_GET_PIPENO\n" );
            substate = 0;
            input_pipeno = 0;
            input_left = input_packet_length;
        }
        return;

    case STATE_GET_PIPENO:
        input_pipeno <<= 8;
        input_pipeno += c;
        if ( ++substate == PACKET_PIPENO_LENGTH )
        {
            state = STATE_GET_BODY;
            printf( "switch to STATE_GET_BODY\n" );
            substate = 0;
            calculated_checksum = 0;
        }
        return;

    case STATE_GET_BODY:
        input_packet[ substate ] = c;
        calculated_checksum += c;
        if ( ++substate == input_packet_length )
        {
            state = STATE_GET_CKSUM;
            printf( "switch to STATE_GET_CKSUM\n" );
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
            printf( "switch to STATE_HDR_HUNT\n" );
            substate = 0;
            if ( input_checksum != calculated_checksum )
            {
                printf( "checksum error %x != %x\n",
                        input_checksum != calculated_checksum );
                unput_flush();
            }
            else
            {
                unput_discard();
                io->outpacket( input_pipeno,
                               input_packet, input_packet_length );
                first_char_after_packet = true;
            }
        }
        return;
    }
}
