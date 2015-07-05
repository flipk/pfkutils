
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

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>

static int
random_main( int hex, int argc, char ** argv )
{
    int num;

    srandom( getpid() * time( NULL ));
    if ( argc != 2 )
        exit( 1 );
    num = atoi( argv[1] );
    while ( num-- > 0 )
    {
        if (hex)
            putchar( "0123456789abcdef"[random()%16] );
        else
            putchar(
 "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
             [random()%62] );
    }
    putchar( '\n' );
    return 0;
}

int
random_text_main( int argc, char ** argv )
{
    return random_main( 0, argc, argv );
}

int
random_hex_main( int argc, char ** argv )
{
    return random_main( 1, argc, argv );
}
