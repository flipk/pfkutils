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
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#include "main.h"

static int
calc_bytes_used( char ** translation, char * startbrk )
{
    int bytes;
    char *endbrk;
    *translation = "";
    endbrk = sbrk( 0 );
    bytes = (long)endbrk - (long)startbrk;
    if ( bytes > 1e7 )
    {
        bytes /= 1e6;
        *translation = "M";
    }
    else if ( bytes > 1e4 )
    {
        bytes /= 1e3;
        *translation = "K";
    }
    
    return bytes;
}

int get_files( int argc, char ** argv, char *** files );

int
maketags_main( int argc, char ** argv )
{
    TAGS_OUTPUT * out;
    FILE * in;
    char * startbrk;
    int i;
    time_t t, t2, starttime;
    int numfiles;
    char ** files;

    numfiles = get_files( argc, argv, &files );

    if ( numfiles < 0 )
        return 1;

    out = maketags_open_output( "tags", TAG_HASH_SIZE,
                                numfiles, files );
    if ( out == NULL )
        return 1;

    printf( "reading input files....\n" );

    startbrk = sbrk( 0 );
    time( &t );
    starttime = t;

    for ( i = 0; i < numfiles; i++ )
    {
        in = fopen( files[i], "r" );
        if ( in == NULL )
        {
            printf( "opening %s: %s\n",
                    files[i], strerror( errno ));
            continue;
        }

        out->current_file_number = i;
        out->current_line = 1;
        maketags_parse_file( in, out );
        fclose( in );
        maketags_output_finish_a_file( out, i );

#define PRINTUSE()                                                      \
        {                                                               \
            int bytes;                                                  \
            char * translation;                                         \
                                                                        \
            bytes = calc_bytes_used( &translation, startbrk );          \
            t = t2;                                                     \
            printf( "\r                          "                      \
                    "                                      \r"          \
                    "%d seconds : %d of %d files, %d tags %d %sbytes ", \
                    (int)(time( NULL ) - starttime),                    \
                    i, numfiles, out->numtags, bytes, translation );    \
            fflush( stdout );                                           \
        }

        if ( time( &t2 ) != t )
            PRINTUSE();
    }


    PRINTUSE();

    printf( "\n" );

    printf( "sorting output.....\n" );
    maketags_sort_output( out );

    {
        char * translation;
        int bytes;

        bytes = calc_bytes_used( &translation, startbrk );
        printf( "writing output.....\n" );
        maketags_close_output( out );
    }

    return 0;
}

int
get_files( int argc, char ** argv, char *** _files )
{
    char curfil[ 80 ];
    char ** files;
    int allocd;  /* number of ptrs allocated */
    int used;    /* number in use */

    if ( argc < 2 )
        return -1;

    argc--;
    argv++;

    if ( argv[0][0] != '-' )
    {
        *_files = argv;
        return argc;
    }

    allocd = 10;
    used = 0;
    files = (char**)xmalloc( sizeof(char*) * allocd );

    while ( fgets( curfil, 80, stdin ))
    {
        char * p;
        for ( p = curfil; *p; p++ )
            if ( *p == '\n' || *p == '\r' )
                *p = 0;
        files[used] = (char*)strdup( curfil );
        if ( ++used == allocd )
        {
            allocd *= 2;
            files = (char**)realloc( files, sizeof(char*) * allocd );
        }
    }

    *_files = files;

    return used;
}
