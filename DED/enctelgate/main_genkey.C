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

#include "d3encdec.H"

#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>

extern "C" int
etg_genkey_main( int argc, char ** argv )
{
    srandom( time(NULL) * getpid() );

    if ( d3des_crypt_genkey() == false )
    {
        fprintf( stderr, "key generation failed\n" );
        return 1;
    }
    return 0;
}
