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
#include <errno.h>
#include <ctype.h>
#include "main.h"

static void   parse_line( TAGS_OUTPUT * to, char * line );
static char * parse_word( TAGS_OUTPUT * to, char * line );

void
maketags_parse_file( FILE * in, TAGS_OUTPUT * out )
{
    char line[ 512 ];

    for ( ; fgets( line, 511, in ) != NULL; out->current_line ++ )
    {
        line[511] = 0;
        parse_line( out, line );
    }    
}

static void
parse_line( TAGS_OUTPUT * to, char * line )
{
    while ( *line != 0 )
    {
        line = parse_word( to, line );
    }
}

static char *
parse_word( TAGS_OUTPUT * to, char * line )
{
    char * wordstart;
    /* walk forward until usable chars are found */
    while ( 1 )
    {
        int c = *line;
        if ( c == 0 )
            return line;
        if ( isalpha( c ) || isdigit( c ) || ( c == '_' ))
            break;
        line++;
    }

    wordstart = line;

    /* walk forward until no more usable chars found */
    while ( 1 )
    {
        int c = *line;
        if ( isalpha( c ) || isdigit( c ) || ( c == '_' ))
        {
            line++;
            continue;
        }
        break;
    }

    /* from worstart to line is a usable word, logit */
    if ( maketags_do_logit( wordstart, line - wordstart ))
        maketags_emit_output( to, wordstart, line - wordstart );

    return line;
}
