
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

#include "mytypes.h"
#include "control_pipe.H"
#include <errno.h>
#include <string.h>
#include <stdio.h>

static uchar * data;
static int datalen;

Control_Pipe :: Control_Pipe( void )
{
    data = NULL;
    datalen = 0;
}

Control_Pipe :: ~Control_Pipe( void )
{
    if ( data )
        delete[] data;
    datalen = 0;
}

int
Control_Pipe :: write( uchar * buf, int  length )
{
    if ( data )
        delete[] data;
    data = new uchar[ length ];
    memcpy( data, buf, length );
    datalen = length;
    return 0;
}

int
Control_Pipe :: read ( uchar * buf, int &length )
{
    if ( !data )
    {
        length = 0;
        return 0;
    }
    if ( length > datalen )
        length = datalen;
    memcpy( buf, data, length );
    delete[] data;
    data = NULL;
    datalen = 0;
    return 0;
}
