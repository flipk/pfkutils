
/*
    This file is part of the "pfkutils" tools written by Phil Knaack
    (pfk@pfk.org).
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

#include "md5.h"
#include <stdio.h>

int
main()
{
    MD5_CTX          ctx;
    unsigned char    digest[16];
    unsigned char    inbuf[ 1024 ];
    unsigned int     len;
    FILE           * f;

    MD5Init( &ctx );

    f = fopen( "md5.c", "r" );

    while ( 1 )
    {
        len = fread( inbuf, 1, sizeof(inbuf), f );
        if ( len == 0 )
            break;
        MD5Update( &ctx, inbuf, len );
    }

    MD5Final( digest, &ctx );

    printf( "digest: " );
    for ( len = 0; len < sizeof(digest); len++ )
    {
        printf( "%02x", digest[len] );
    }
    printf( "\n" );

    return 0;
}
