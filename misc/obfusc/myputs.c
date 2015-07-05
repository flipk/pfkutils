
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

#include "myputs.h"

int col = 0;
int last_was_newline=1;
int define_line = 0;
int last_was_whitespace = 0;

void
myputs( char * s )
{
    int len = strlen(s);
    printf( "%s", s );
    last_was_newline=0;
    col += len;
    last_was_whitespace=0;
}

void
myputwhitespace( void )
{
    if ( last_was_whitespace==1 )
        return;

    if ( col > 60 )
    {
        if ( last_was_newline == 0 )
        {
            if ( define_line )
                printf( " \\" );
            putchar( '\n' );
            last_was_newline=1;
            last_was_whitespace=1;
        }
        col = 0;
    }
    else
    {
        if ( col > 0 )
        {
            putchar( ' ' );
            col++;
            last_was_whitespace=1;
        }
        last_was_newline=0;
    }
}

void
myputpreproc( char * s )
{
    if ( col != 0 )
    {
        if ( last_was_newline == 0 )
        {
            if ( define_line )
                printf( " \\" );
            putchar( '\n' );
        }
        col=0;
    }
    printf( "%s\n", s );
    last_was_newline=1;
    last_was_whitespace=1;
}


extern char *yytext;
extern int yyleng;

void
consume_preproc( int (*func)(void) )
{
    char token[100];
    int prev_c;

    memcpy(token,yytext,yyleng);
    token[yyleng]=0;

    if ( col != 0 )
    {
        if ( last_was_newline == 0 )
        {
            if ( define_line )
                printf( " \\" );
            putchar( '\n' );
        }
        col=0;
    }
    last_was_newline=1;
    printf( "%s", token );

    prev_c = -1;
    while ( 1 )
    {
        int c = func();
        putchar(c);
        if ( c == 0 )
            return;
        if ( c == 13 || c == 10 )
        {
            if ( prev_c != '\\' )
                return;
        }
        prev_c = c;
    }
    last_was_whitespace=1;
}

void
consume_define( int (*func)(void) )
{
    char token[100];

    memcpy(token,yytext,yyleng);
    token[yyleng]=0;

    if ( col != 0 )
    {
        if ( last_was_newline == 0 )
        {
            if ( define_line )
                printf( " \\" );
            putchar( '\n' );
        }
        col=0;
    }
    last_was_newline=1;
    printf( "%s ", token );
    define_line=1;
    last_was_whitespace=0;
}
