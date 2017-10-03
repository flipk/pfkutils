
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

void
usage()
{
    fprintf( stderr,
             "usage:  tmpl_to_c [template_file var_name]...\n" );
    exit( 1 );
}

void process_template( char * file, char * var );

int
main( int argc, char ** argv )
{
    argc--; argv++;
    if ( argc == 0 || (( argc & 1 ) != 0 ))
        usage();

    do
    {
        fprintf( stderr, "processing template file %s\n", argv[0] );
        process_template( argv[0], argv[1] );
        argc -= 2;
        argv += 2;
    }
    while ( argc > 0 );

    return 0;
}

void
outline( char * s )
{
    while (1)
    {
        if ( *s == 10 || *s == 13 || *s == 0 )
            return;
        if ( *s == '"' )
            putchar( '\\' );
        if ( *s == '\\' )
            putchar( '\\' );
        putchar( *s );
        s++;
    }
}

void
process_template( char * file, char * var )
{
    char line[132];
    FILE * f;

    f = fopen( file, "r" );
    if ( !f )
    {
        fprintf( stderr, "could not open file %s\n", file );
        exit( 1 );
    }

    printf( "const char * %s = \n", var );

    while ( fgets( line, 131, f ))
    {
        if ( line[0] == '%' )  /* handle comments */
            continue;
        putchar( '"' );
        outline( line );
        printf( "\\n\"\n" );
    }

    printf( ";\n" );
}
