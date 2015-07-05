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

/** \file md5buffer.cc
 * \brief utility functions to deal with md5 hashes.
 * \author Phillip F Knaack
 */
#include "database_elements.h"
#include "params.h"
#include "protos.h"

#include <pk-md5.h>

/** calculate md5 hash for a buffer of bytes.
 *
 * @param buf   pointer to a buffer of data.
 * @param buflen number of bytes in the buf pointer.
 * @param md5    pointer to an md5hash buffer to be filled in by this
 *               function.
 */
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

/** convert an md5 hash into an ascii string for debug displays.
 * 
 * @param md5hash  pointer to an md5 hash
 * @param string   pointer to a buffer of size MD5_STRING_LEN which 
 *                 will be filled by this function with an ascii string
 *                 of hex digits, terminated with a nul (no newline).
 */
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
