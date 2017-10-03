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
incs="-I ../pkutils"
flags="-g3"
g++ $incs $flags -c d3test.C
g++ $incs $flags -c d3encdec.C
g++ $incs $flags -c d3des.C
g++ $incs $flags d3des.o d3encdec.o d3test.o -o d3test
exit
;
#endif

#include "d3encdec.H"

#include <stdio.h>
#include <string.h>

void
redclear( unsigned char *red1,
          unsigned char *red2,
          unsigned char *red3 )
{
    memset( red1, 0xee, 32 );
    memset( red2, 0xee, 32 );
    memset( red3, 0xee, 32 );
}

void
redcheck( unsigned char *red1,
          unsigned char *red2,
          unsigned char *red3, char *when )
{
    int j;
    for ( j = 0; j < 32; j++ )
        if ( red1[j] != 0xee )
            printf( "%s: red1[%d] = %02x\n", when, j, red1[j] );
    for ( j = 0; j < 32; j++ )
        if ( red2[j] != 0xee )
            printf( "%s: red2[%d] = %02x\n", when, j, red2[j] );
    for ( j = 0; j < 32; j++ )
        if ( red3[j] != 0xee )
            printf( "%s: red3[%d] = %02x\n", when, j, red3[j] );
}

int
main()
{
    d3des_crypt * dc;

    const unsigned char key[8] = { 1,2,3,4,5,6,7,8 };
    dc = new d3des_crypt( key );

    unsigned char red1[ 32 ];
    unsigned char data[ 32 ];
    unsigned char red2[ 32 ];
    unsigned char enc [ 32+8 ];
    unsigned char red3[ 32 ];
    int enc_len, dec_len;
    int i, j;

    for ( i = 1; i <= sizeof(data); i++ )
    {
        for ( j = 0; j < i; j++ )
            data[j] = j+1;
        redclear(red1,red2,red3);
        dc->encrypt_packet( data, i, enc, &enc_len );
        redcheck(red1,red2,red3,"encrypt");
        printf( "encrypt %d to %d\nin  = ", i, enc_len );
        for ( j = 0; j < i; j++ )
            printf( "%02x", data[j] );
        printf( "\nout = " );
        for ( j = 0; j < enc_len; j++ )
            printf( "%02x", enc[j] );
        printf( "\nin  = " );
        redclear(red1,red2,red3);
        dc->decrypt_packet( enc, enc_len, data, &dec_len );
        redcheck(red1,red2,red3,"decrypt");
        for ( j = 0; j < dec_len; j++ )
            printf( "%02x", data[j] );
        printf( "\n" );
    }
        
    delete dc;
    return 0;
}
