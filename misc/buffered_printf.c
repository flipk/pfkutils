
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

#if 0
ccsimso -I /usr/test/bssdata2/tornado3/vxsim20.0/target/h -DCPU=SIMSPARCSOLARIS -c buffered_printf.c
exit 0
#endif

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <fioLib.h>

#define BUFFER_SIZE 512

static char print_buf[ BUFFER_SIZE+1 ];
static int  print_buf_pos = 0;

#define LEFT() (BUFFER_SIZE - print_buf_pos)

void
flush_buffered_printf( void )
{
    if ( print_buf_pos > 0 )
    {
        /* note that print_buf is 1 larger than BUFFER_SIZE
           so it is legal to do this write even if 
           print_buf_pos==BUFFER_SIZE */
        print_buf[print_buf_pos] = 0;
        lprintf( EXEC_MON_TERM_ID, "%s", print_buf );
    }
    print_buf_pos = 0;
}

static int
printout( char * buffer, int nchars, int outarg )
{
    while ( nchars > 0 )
    {
        int cpy = nchars;
        if ( cpy > LEFT() )
            cpy = LEFT();

        memcpy( print_buf + print_buf_pos, buffer, cpy );

        print_buf_pos += cpy;
        if ( print_buf_pos == BUFFER_SIZE )
            flush_buffered_printf();

        buffer += cpy;
        nchars -= cpy;
    }
    return OK;
}

void
buffered_printf( char * format, ... )
{
    va_list ap;

    va_start( ap, format );
    fioFormatV( format, ap, printout, 0 );
    va_end( ap );
}
