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

#ifndef __PACKET_H__
#define __PACKET_H__

#include "types.H"

/*
 * a PFK1-encoded packet has the following layout:
 *
 *  (1) the 4 characters "PFK1"
 *  (2) 1 or more 4-character groups of base-64-encoded data containing:
 *      (2a) 2 bytes for 'data' length
 *      (2b) N bytes of 'data' (see length field) containing encrypted data:
 *          (2b1) 2 bytes for real data length
 *          (2b2) N bytes of real data according to (2b1)
 *      (2c) 2 bytes for 'checksum'
 *  (3) according to base64 standard, if the data contained in
 *      bullet (2) is not a multiple of 3 bytes long, the 4-byte
 *      base64 group is filled out by '=' character.
 *
 *  while encoding the data, a newline '\n' is inserted every 72
 *  encoded characters in case any intermediate transmission equipment
 *  is susceptible to data corruption due to line terminator awareness.
 */

#define PACKET_MAX_PAYLOAD_DATA 1600
#define PACKET_MAX_B64_DATA     (PACKET_MAX_PAYLOAD_DATA*4/3+4)

#define PACKET_HEADER_STRING    "PFK1"
#define PACKET_HEADER_LENGTH    (sizeof(PACKET_HEADER_STRING)-1)
#define PACKET_LENGTH_LENGTH    2
#define PACKET_PIPENO_LENGTH    2
#define PACKET_CHECKSUM_LENGTH  2

#define NUM_CHARS_BETWEEN_NEWLINES  72
#define NUM_NEWLINES ((PACKET_MAX_B64_DATA / NUM_CHARS_BETWEEN_NEWLINES) + 4)
#define PACKET_ENCODED_MAX_LENGTH \
   (PACKET_MAX_B64_DATA + PACKET_HEADER_LENGTH + \
    PACKET_LENGTH_LENGTH + PACKET_PIPENO_LENGTH + \
    PACKET_CHECKSUM_LENGTH + NUM_NEWLINES + 4)

struct etg_stats {
    int out_packets;
    int out_bytes;
    int in_packets;
    int in_bytes;
};

extern struct etg_stats stats;
void print_stats( void );

class packet_encrypt_decrypt_io {
public:
    virtual ~packet_encrypt_decrypt_io( void ) { /* placeholder */ }
    virtual bool decrypt_packet( unsigned char * in,  int in_len,
                                 unsigned char * out, int * out_len ) = 0;
    virtual void encrypt_packet( unsigned char * in,  int in_len,
                                 unsigned char * out, int * out_len ) = 0;
};

#endif /* __PACKET_H__ */
