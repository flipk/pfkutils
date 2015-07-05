
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

#include <pfkutils_config.h>

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>

#include "pk-md5.h"

#define NEW(x,y)  (x *)calloc(y, sizeof(x))
#define FREE(x)  { if (x != NULL) free(x); x = NULL; }

static char *
dirpart( char * x )
{
    char *ret = strrchr(x, '/');
    if (ret)
        *ret = 0;
    return ret ? ret+1 : x;
}

static char *
lastpart( char * x )
{
    char *ret = strrchr(x, '/');
    return ret ? ret+1 : x;
}

static void
recurse( char * name )
{
    DIR *cur;
    struct dirent *de;

    cur = opendir( name );
    if ( !cur )
        return;
    
    printf( "D %s\n", name );

    while ( de = readdir( cur ))
    {
        char *s;
        char *ent = de->d_name;
        int cc;
        static char buf[1024];
        enum { TYPE_FILE, TYPE_DIR, TYPE_LINK } type;

#ifndef HAVE_STRUCT_DIRENT_D_TYPE
        /* some OSs don't provide a 'type' field in dirents,
           so we have to stat to know what they are. */
        struct stat sb;
#endif

        s = NEW( char, strlen(ent)+strlen(name)+2 );
        sprintf( s, "%s/%s", name, ent );

#ifdef HAVE_STRUCT_DIRENT_D_TYPE
        if ( de->d_type == DT_DIR )
            type = TYPE_DIR;
        else if ( de->d_type == DT_LNK )
            type = TYPE_LINK;
        else
            type = TYPE_FILE;
#else
        if ( stat( s, &sb ) < 0 )
        {
            fprintf( stderr, "error in stat '%s': %s\n",
                     s, strerror( errno ));
            FREE(s);
            continue;
        }
        if ( S_ISDIR(sb.st_mode))
            type = TYPE_DIR;
        else if ( S_ISLNK(sb.st_mode))
            type = TYPE_LINK;
        else
            type = TYPE_FILE;
#endif

        if (( type == TYPE_DIR ) &&
            (( strcmp( ent, "." ) == 0 ) ||
             ( strcmp( ent, ".." ) == 0 )))
        {
            FREE(s);
            continue;
        }

        switch ( type )
        {
        case TYPE_DIR:
            recurse( s );
            break;

        case TYPE_LINK:
            cc = readlink( s, buf, 1023 );
            if ( cc < 0 )
            {
                printf( "readlink %s error %s\n",
                        s, strerror( errno ));
                break;
            }
            buf[cc] = 0;
            printf( "L %s '%s'\n", s, buf );
            break;

        case TYPE_FILE:
            MD5File( s, buf );
            printf( "F %s\nH %s %s\n", s, s, buf );
            break;
        }

        FREE( s );
    }
    closedir( cur );
}

static void
usage( int f, char * msg )
{
    if ( f & 2 )
        perror( msg );
    else
        fprintf( stderr,
                 "usage: netbsd-scan source-dir [source-dir ..]\n" );
    if ( f & 1 )
        exit( 1 );
}

int
treescan_main( int argc, char ** argv )
{
    char *p;

    if ( argc < 2 )
        usage( 1,NULL );

    argv += 1;
    argc -= 1;

    if ( !argc )
        usage( 1,NULL );

    while ( argc )
    {
        char *f = dirpart( argv[0] );
        if ( chdir( argv[0] ))
            usage( 2,"chdir" );
        recurse( f );
        argc--;
        argv++;
    }

    return 0;
}
