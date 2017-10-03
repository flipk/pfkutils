/*
    This file is part of the "pkutils" tools written by Phil Knaack
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

#if 0
set -e -x
gcc -O3 encrypt.c timing.c keyfile.c gmp-4.1.2/.libs/libgmp.a -o e
strip e
exit 0
#endif

#include <stdio.h>
#include "gmp.h"
#include "keyfile.h"
#include "timing.h"

#define VERBOSE 0

// file layout:
//   each block preceded by one byte marker
//   marker   contents
//     b       bytes per block
//     p       block data
//     e       bytes in last block

int
main( int argc, char ** argv )
{
    mpz_t   n, e, m, c;
    int     bytes, i, cc;
    char  * buffer;
    char  * bufferx;
    TS_VAR  ts;

    mpz_init( n );
    mpz_init( e );
    mpz_init( m );
    mpz_init( c );

    if ( getkeys_pub( n, e, &bytes ) != 0 )
        return 1;

    buffer  = (char*) malloc( bytes   );
    bufferx = (char*) malloc( bytes*2 );

#if VERBOSE
    mpz_get_str( bufferx, 16, e );
    fprintf( stderr, "e = %s\n", bufferx );
    mpz_get_str( bufferx, 16, n );
    fprintf( stderr, "n = %s\n", bufferx );
#endif

    TS_START(ts);

    // m is message
    // c is cipher

    fprintf( stdout, "b%d\n", bytes );
    
    while ( 1 )
    {
        // read input from fd 0
        cc = read( 0, buffer, bytes );
        if ( cc <= 0 )
            break;
        if ( cc != bytes )
            fprintf( stdout, "e%d\n", cc );

        // encode buffer into m
        for ( i = 0; i < cc; i++ )
            sprintf( bufferx + i*2, "%02x", (unsigned char) buffer[i] );

        mpz_set_str( m, bufferx, 16 );

#if VERBOSE
        mpz_get_str( bufferx, 16, m );
        fprintf( stderr, "m = %s\n", bufferx );
#endif

        // encrypt m to c
        mpz_powm( c, m, e, n );

#if VERBOSE
        mpz_get_str( bufferx, 16, c );
        fprintf( stderr, "c = %s\n", bufferx );
#endif

        // output cipher text
        fputc( 'p', stdout );
        mpz_out_raw( stdout, c );
    }

    fprintf( stderr, "encrypt time = %d ms\n", TS_END(ts) );

    free( buffer  );
    free( bufferx );
    mpz_clear( n );
    mpz_clear( e );
    mpz_clear( m );
    mpz_clear( c );
    return 0;
}
