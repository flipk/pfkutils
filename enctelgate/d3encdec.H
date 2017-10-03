/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

#ifndef __D3ENCDEC_H__
#define __D3ENCDEC_H__

#include "types.H"
#include "packet_encoder.H"
#include "packet_decoder.H"

extern "C" {
#include "d3des.H"
};

class d3des_crypt : public packet_encrypt_decrypt_io {
    DES_CRYPT   crypt;
public:
    d3des_crypt( const unsigned char * hexkey )  // [8]
        : crypt( hexkey ) { /* nothing */ }
    /*virtual*/ ~d3des_crypt( void ) { /* nothing */ }
    /*virtual*/ void encrypt_packet( unsigned char * in,  int    in_len,
                                     unsigned char * out, int * out_len );
    /*virtual*/ bool decrypt_packet( unsigned char * in,  int    in_len,
                                     unsigned char * out, int * out_len );
};

// return false if file could not be written
bool d3des_crypt_genkey( void );
// return NULL if no key found
d3des_crypt * d3des_crypt_loadkey( void );

#endif /* __D3ENCDEC_H__ */
