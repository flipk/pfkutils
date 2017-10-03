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

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "packet_decoder.H"
#include "packet_encoder.H"
#include "base64.h"

struct etg_stats stats;

static time_t  last_print;

void
print_stats( void )
{
    time_t now = time(NULL);
    if ( last_print == now )
        return;
    last_print = now;
    printf( "\r  tx packets %d bytes %d  rx packets %d bytes %d\r",
            stats.out_packets, stats.out_bytes,
            stats.in_packets,  stats.in_bytes );
    fflush( stdout );
}
