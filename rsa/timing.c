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

#include "timing.h"

int
_ts_endtime( struct timeval * s )
{
    struct timeval e, d;

    gettimeofday( &e, 0 );

    d.tv_sec  = e.tv_sec  - s->tv_sec;
    d.tv_usec = e.tv_usec - s->tv_usec;

    if ( d.tv_usec < 0 )
    {
        d.tv_usec += 1000000;
        d.tv_sec -= 1;
    }

    return d.tv_usec / 1000 + d.tv_sec * 1000;
}
