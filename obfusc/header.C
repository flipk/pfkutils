
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
#include <sys/stat.h>
#include <stdlib.h>
#include <stdarg.h>

extern "C" {
#include "tokens.h"
#include "myputs.h"
};

#include "btree.H"
#include "translate.h"

static char *
spr( void * arg, int noderec,
     int keyrec, void * key, int keylen,
     int datrec, void * dat, int datlen )
{
    int len = 
        sizeof( "#define   " ) +
        keylen + datlen;
    char * ret;
    ret = new char[len];

    if ( strncmp( (char*)key, "__phil_k_", 9 ) == 0    ||
         ((char*)dat)[0] == 1 )
        ret[0] = 0;
    else
        sprintf( ret, "#define %s %s\n", (char*)dat+1, (char*)key );

    return ret;
}

static void
sprf( void * arg, char * s )
{
    delete[] s;
}

static void
pr( void * arg, char * format, ... )
{
    FILE * f = (FILE*) arg;
    va_list  args;

    va_start( args, format );
    vfprintf( f, format, args );
    va_end( args );
}

void
produce_header( Btree * bt )
{
    FILE * header;

    header = fopen( "transtags.h", "w" );

    Btree::printinfo  pi = {
        &spr,
        &sprf,
        &pr,
        (void*) header,
        false
    };

    bt->dumptree( &pi );

    fclose( header );
}
