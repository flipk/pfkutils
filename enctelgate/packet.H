
#ifndef __PACKET_H__
#define __PACKET_H__

#include "types.H"

/*
 * a PFK1-encoded packet has the following layout:
 *
 *  (1) the 4 characters "PFK1"
 *  (2) 1 or more 4-character groups of base-64-encoded data containing:
 *      (2a) 2 bytes for 'data' length
 *      (2b) N bytes of 'data' (see length field)
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

#endif /* __PACKET_H__ */
