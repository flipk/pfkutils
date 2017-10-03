
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
#include <stdlib.h>

/*
 * usage:
 *    fmtsdiff $width
 *
 * read stdin, looking for "|", "<", and ">" in the appropriate column
 * of an sdiff output (given the width of the current terminal), and for
 * each of these lines, output a marker ">" at the start of that line;
 * this makes it easy for 'less' to highlight changed lines.
 */

#define GUTTER_WIDTH_MINIMUM 3
#define min(a,b) ((a) <= (b) ? (a) : (b))
#define max(a,b) ((a) >= (b) ? (a) : (b))
#define isdiffchar(c) ((c)=='|'||(c)=='<'||(c)=='>')

int
fmtsdiff_main( int argc, char ** argv )
{
    int width;
    int sdiff_half_width;
    int sdiff_column2_offset;
    char * line;

    if ( argc != 2 )
        return 1;

    width = atoi( argv[1] );

    /* this math duplicates that found in GNU diff */

    sdiff_column2_offset = (width + 2 + GUTTER_WIDTH_MINIMUM) / 2;
    sdiff_half_width = 
        max( 0,
             min (sdiff_column2_offset - GUTTER_WIDTH_MINIMUM,
                  width - sdiff_column2_offset));

    sdiff_column2_offset -= 3;
    width += 30;

    line = (char*) malloc( width );

    while ( 1 )
    {
        char * cp, c = 0;
        int isdiffline;

        if ( fgets( line, width-1, stdin ) == NULL )
            break;

        line[width-1] = 0;
        cp = strchr( line, '\n' );
        if ( cp )
            *cp = 0;

        isdiffline = ( strlen(line) > sdiff_column2_offset );
        if ( isdiffline )
            c = line[sdiff_column2_offset];

        if ( isdiffchar( c ) && isdiffline )
        {
            line[sdiff_column2_offset-1] = 0;
            printf( "%s%c%c%c%s\n",
                    line, '%', c, '%', line + sdiff_column2_offset + 2 );
        }
        else
            printf( "%s\n", line );
    }

    free( line );
    return 0;
}
