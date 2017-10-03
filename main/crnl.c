
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

/*
 * (c-set-style "BSD")
 *
 * carriage-return line-feed converter
 * 
 * this tool converts text files between dos, unix, and mac formats.
 * its like dos2unix except it supports all 3 formats, to and from,
 * and leaves the original file behind with a ~ on the end.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
usage( void )
{
    printf(
        "usage:\n"
        "   this program converts from one text file format to another\n"
        "   - unix files use only 0x0A for newline\n"
        "   - macintosh files use 0x0D for newline\n"
        "   - msdos files use 0x0D 0x0A for newline\n"
        "   - syntax: crnl <from_format> <to_format> file [file ...]\n"
        "   - where 'format' is one of: -dos -unix -mac\n"
        );
    exit( 1 );
}

typedef int  (*read_filter) ( FILE *, char *, int );
typedef void (*write_filter)( FILE *, char *, int );

static int warnings = 0;
#define IFWARN(x) (!(warnings & (1<<x)))
#define NOWARN(x) (warnings |= (1<<x))

static int
get_dos( FILE * f, char * buf, int max )
{
    int r = 0;
    int c;
    while ( 1 )
    {
        c = fgetc( f );
        if ( c == EOF )
        {
            if ( r != 0 )
            {
                if (IFWARN(0))
                {
                    fprintf( stderr, "warning, last line has no newline\n" );
                    NOWARN(0);
                }
                return r;
            }
            return -1;
        }
        if ( c == 0x0A )
        {
            if (IFWARN(1))
            {
                fprintf( stderr, "warning, found 0x0A without preceding "
                         "0x0D, is this really a dos file?\n" );
                NOWARN(1);
            }
        }
        if ( c == 0x0D )
        {
            c = fgetc( f );
            if ( c != 0x0A )
            {
                if (IFWARN(2))
                {
                    fprintf( stderr, "error, 0x0A without 0x0D, "
                             "is this really a dos file?\n" );
                    NOWARN(2);
                }
            }
            return r;
        }
        buf[r] = (char)c;
        if ( ++r == max )
        {
            if (IFWARN(3))
            {
                fprintf( stderr, "error, found line > %d chars long\n", max );
                NOWARN(3);
            }
        }
    }
}

static int
get_mac( FILE * f, char * buf, int max )
{
    int r = 0;
    int c;
    while ( 1 )
    {
        c = fgetc( f );
        if ( c == EOF )
        {
            if ( r != 0 )
            {
                if (IFWARN(4))
                {
                    fprintf( stderr, "warning, last line has no newline\n" );
                    NOWARN(4);
                }
                return r;
            }
            return -1;
        }
        if ( c == 0x0A )
        {
            if (IFWARN(5))
            {
                fprintf( stderr, "warning, 0x0A character found in "
                         "input, is this really a mac file?\n" );
                NOWARN(5);
            }
        }
        if ( c == 0x0D )
            return r;
        buf[r] = (char)c;
        if ( ++r == max )
        {
            if (IFWARN(6))
            {
                fprintf( stderr, "error, found line > %d chars long\n", max );
                NOWARN(6);
            }
        }
    }
}

static int
get_unix( FILE * f, char * buf, int max )
{
    int r = 0;
    int c;
    while ( 1 )
    {
        c = fgetc( f );
        if ( c == EOF )
        {
            if ( r != 0 )
            {
                if (IFWARN(7))
                {
                    fprintf( stderr, "warning, last line has no newline\n" );
                    NOWARN(7);
                }
                return r;
            }
            return -1;
        }
        if ( c == 0x0D )
        {
            if (IFWARN(8))
            {
                fprintf( stderr, "warning, 0x0D character found in "
                         "input, is this really a unix file?\n" );
                NOWARN(8);
            }
        }
        if ( c == 0x0A )
            return r;
        buf[r] = (char)c;
        if ( ++r == max )
        {
            if (IFWARN(9))
            {
                fprintf( stderr, "error, found line > %d chars long\n", max );
                NOWARN(9);
            }
        }
    }
}

static void
put_dos( FILE * f, char * buf, int len )
{
    buf[len] = 0x0d;
    buf[len+1] = 0x0a;
    fwrite( buf, len+2, 1, f );
}

static void
put_mac( FILE * f, char * buf, int len )
{
    buf[len] = 0x0d;
    fwrite( buf, len+1, 1, f );
}

static void
put_unix( FILE * f, char * buf, int len )
{
    buf[len] = 0x0a;
    fwrite( buf, len+1, 1, f );
}

static void
convert_file( char * file, read_filter r, write_filter w )
{
    FILE * inf, * outf;
    char buffer[512];
    int len;

    sprintf( buffer, "%s~", file );
    if ( rename( file, buffer ) < 0 )
    {
        fprintf( stderr, "error, cannot rename file '%s'\n", file );
        exit( 1 );
    }
    inf = fopen( buffer, "r" );
    if ( inf == NULL )
    {
        fprintf( stderr, "error, cannot open input file '%s'\n", buffer );
        exit( 1 );
    }
    outf = fopen( file, "w" );
    if ( outf == NULL )
    {
        fprintf( stderr, "error, cannot open output file '%s'\n", file );
        exit( 1 );
    }

    while (( len = r( inf, buffer, 510 )) != -1 )
        w( outf, buffer, len );

    fclose( inf );
    fclose( outf );
}

static read_filter  readers[3] = { get_dos, get_mac, get_unix };
static write_filter writers[3] = { put_dos, put_mac, put_unix };

static int
arg_to_index( char * arg )
{
    if ( strcmp( arg, "-dos" ) == 0 )
        return 0;
    if ( strcmp( arg, "-mac" ) == 0 )
        return 1;
    if ( strcmp( arg, "-unix" ) == 0 )
        return 2;
    return -1;
}

int
crnl_main( int argc, char ** argv )
{
    int r_ind, w_ind;
    int i;

    if ( argc < 4 )
        usage();

    r_ind = arg_to_index( argv[1] );
    w_ind = arg_to_index( argv[2] );

    if ( r_ind == -1 || w_ind == -1 )
        usage();

    if ( r_ind == w_ind )
    {
        fprintf( stderr, "converting %s to %s is a pointless conversion\n",
                 argv[1], argv[2] );
        return -1;
    }

    for ( i = 3; i < argc; i++ )
        convert_file( argv[i], readers[r_ind], writers[w_ind] );

    return 0;
}
