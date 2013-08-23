
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

#include "base64.h"

static inline unsigned char
value_to_b64( int v )
{
    static unsigned char table[] = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    if ( v >= sizeof(table)   ||   v < 0 )
        return -1;
    return table[ v ];
}

static inline int
b64_to_value( unsigned char b64 )
{
    static char table[] = {   /* table starts with char 0x20 */
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
        -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
        -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51
    };
    if ( b64 < 0x20 )
        return -1;
    b64 -= 0x20;
    if ( b64 >= sizeof(table) )
        return -1;
    return table[ b64 ];
}

int
b64_is_valid_char( unsigned char c )
{
    if ( c == '=' )
        return 1;
    if ( b64_to_value(c) == -1 )
        return 0;
    return 1;
}

/* return 4 if ok, 0 if not ok */
int
b64_encode_quantum( unsigned char * in3, int in_len, unsigned char * out4 )
{
    int v;
    unsigned int val;

    if ( in_len > 3  || in_len < 1 )
        return 0;

    val = (in3[0] << 16);
    if (in_len > 1)
        val |= (in3[1] << 8);
    if (in_len == 3)
        val |= in3[2];

#define TRY(tlen,index,shift) \
    do { \
        if ( in_len > tlen ) \
        { \
            v = (val >> shift) & 0x3f; \
            v = value_to_b64( v ); \
            if ( v == -1 ) \
                return 0; \
            out4[index] = v; \
        } \
        else \
        { \
            out4[index] = '='; \
        } \
    } while (0)

    TRY(0,0,18);
    TRY(0,1,12);
    TRY(1,2,6);
    TRY(2,3,0);

    return 4;
}

/* return length of bytes decoded, or 0 if not ok */
int
b64_decode_quantum( unsigned char * in4, unsigned char * out3 )
{
    int val=0,v;
    int ret;

    v = b64_to_value(in4[0]);
    if ( v == -1 ) return 0;
    val += (v << 18);

    v = b64_to_value(in4[1]);
    if ( v == -1 ) return 0;
    val += (v << 12);

    if ( in4[2] == '='   &&  in4[3] == '=' )
    {
        ret = 1;
        goto unpack;
    }

    v = b64_to_value(in4[2]);
    if ( v == -1 ) return 0;
    val += (v << 6);

    if ( in4[3] == '=' )
    {
        ret = 2;
        goto unpack;
    }

    v = b64_to_value(in4[3]);
    if ( v == -1 ) return 0;
    val += (v << 0);

    ret = 3;

 unpack:
    out3[0] = (val >> 16) & 0xff;
    out3[1] = (val >>  8) & 0xff;
    out3[2] = (val >>  0) & 0xff;

    return ret;
}
