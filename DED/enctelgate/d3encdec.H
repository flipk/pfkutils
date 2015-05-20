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
