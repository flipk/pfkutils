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

#include "database_elements.H"
#include "params.H"
#include "protos.H"

#include <pk-md5.h>

void
pfkbak_md5_buffer( UCHAR * buf, int buflen, UCHAR * md5 )
{
    MD5_CTX ctx;
    MD5_DIGEST     digest;

    MD5Init( &ctx );
    MD5Update( &ctx, buf, buflen );
    MD5Final( &digest, &ctx );

    memcpy(md5, digest.digest, MD5_DIGEST_SIZE);
}

void
pfkbak_sprint_md5  ( UCHAR * md5hash, char * string )
{
    int i, cc;
    for (i = 0; i < MD5_DIGEST_SIZE; i++)
    {
        cc = sprintf(string, "%02x", md5hash[i]);
        string += cc;
    }
}
