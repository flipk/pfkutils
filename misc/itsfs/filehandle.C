
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

#include <stdio.h>
#include <string.h>

#include "filehandle.H"
#include "config.h"

bool
FileHandle :: decode( encrypt_iface * crypt, nfs_fh *buffer )
{
#ifdef USE_CRYPT
    if ( crypt )
        crypt->decrypt( (UCHAR*)this, buffer->data, FH_SIZE );
    else
#endif
        memcpy( (UCHAR*)this, buffer->data, FH_SIZE );
    return valid();
}

void
FileHandle :: encode( encrypt_iface * crypt, nfs_fh *buffer )
{
    magic.set( MAGIC );
    checksum.set( calc_checksum() );
#ifdef USE_CRYPT
    if ( crypt )
        crypt->encrypt( buffer->data, (UCHAR*)this, FH_SIZE );
    else
#endif
        memcpy( buffer->data, (UCHAR*)this, FH_SIZE );
}

UINT32
FileHandle :: calc_checksum( void )
{
    UINT32 old_checksum, sum;
    int count;
    uchar * ptr;

    old_checksum = checksum.get();
    checksum.set( 0 );

    sum = 0;
    ptr = (uchar*)this;

    for ( count = 0; count < FH_SIZE; count++ )
    {
        sum += *ptr++;
    }

    checksum.set( old_checksum );

    return sum + SUM_CONSTANT;
}
