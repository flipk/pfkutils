
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

#include "memsrch.h"

/* search a big buffer for occurrances of bytes
   specified in little buffer */
int
memsrch( uchar * bigbuf, uint biglen,
         uchar * littlebuf, uint littlelen )
{
    int i, j, ret = -1;
    if ( biglen == 0 || littlelen > biglen )
        return -1;
    for ( j = i = 0; i < biglen; i++ )
    {
        while ( littlebuf[j] == bigbuf[i] )
        {
            if ( j == 0 )
                ret = i;
            j++; i++;
            if ( j == littlelen )
                return ret;
        }
        if ( ret >= 0 )
        {
            i = ret; j = 0;
        }
        ret = -1;
    }
    return ret;
}
