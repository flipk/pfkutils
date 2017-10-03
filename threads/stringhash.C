
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

#include "stringhash.H"

// #include <stdio.h>

int
string_hash( char * string )
{
    int ret = 5, mult = 1; char * p = string;
    for ( ; *p; p++ )
    {
        ret += ( (int)(*p) * mult );
        mult++;
    }
//    fprintf( stderr, "string '%s' hashes to key %d\n", string, ret );
    return ret;
}
