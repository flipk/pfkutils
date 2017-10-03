
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

#include <string.h>
#include <stdio.h>

#include "main.h"

/**/   char viewtags_input_line[ MAXLINE ];
static char input_line_parsed  [ MAXLINE ];
/**/   char * viewtags_lineargs[ MAXARGS ];

int
viewtags_get_line( FILE * f )
{
    int i;
    char * line;

    if ( fgets( viewtags_input_line, MAXLINE-1, f ) == NULL )
        return -1;

    for ( line = viewtags_input_line; *line; line++ )
        if ( *line == '\n' || *line == '\r' )
        {
            *line = 0;
            break;
        }

    strcpy( input_line_parsed, viewtags_input_line );

    line = input_line_parsed;
    for ( i = 0; i < MAXARGS; i++ )
    {
        viewtags_lineargs[i] = line;

        /* walk forward until space found */
        while ( *line != ' ' && *line != 0 )
            line++;

        if ( *line == 0 )
            return i+1;

        *line++ = 0;

        /* walk forward until nonspace found */
        while ( *line == ' ' )
            line++;

        if ( *line == 0 )
            return i+1;
    }

    return i;
}
