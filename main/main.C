
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

#include <pfkutils_config.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <errno.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>

static char * argv_zero;

struct prog_table {
    const char * program;
    int (*mainfunc)( int argc, char ** argv );
    int priv;
    const char * help;
};

#define PROG(priv,name,help) \
    extern "C" int name##_main( int argc, char ** argv );
#include "programs.h"
#undef PROG

static struct prog_table prog_table[] = {

#define PROG(priv,name,help) { #name, &name##_main, priv, help },
#include "programs.h"
#undef PROG

    { NULL, NULL, 0 }
};

static void
unlink_all_links( void )
{
    DIR * d;
    struct dirent * de;

    d = opendir( "." );
    if ( d == NULL )
    {
        printf( "opendir of '.' failed (%s)\n", strerror( errno ));
        exit( 1 );
    }

    while (( de = readdir( d )) != NULL )
    {
        char p[ 1024 ];
        int count;

        count = readlink( de->d_name, p, 1023 );
        if ( count < 0 )
            continue;

        p[count] = 0;

        if ( strncmp( p, PACKAGE_NAME, strlen(PACKAGE_NAME) ) == 0 )
            if ( unlink( de->d_name ) < 0 )
                printf( "unlink %s failed (%s)\n",
                        de->d_name, strerror( errno ));
    }

    closedir( d );
}

static void
remove_privs( void )
{
    seteuid( getuid() );
}

static int
handle_admin( int argc, char ** argv )
{
    printf("version: %s\n", PACKAGE_STRING);
    if ( argc == 0 || 
         strcmp( argv[0], "-help" ) == 0 )
    {
    usage:
        printf( "usage: pfkutils [-dellinks] [-makelinks] "
                "[-help]\n" );
        return -2;
    }

    if ( strcmp( argv[0], "-dellinks" ) == 0 )
    {
        unlink_all_links();
        return 0;
    }

    if ( strcmp( argv[0], "-makelinks" ) == 0 )
    {
        struct prog_table * pt;
        struct stat sb;

        unlink_all_links();

        for ( pt = prog_table; pt->program != NULL; pt++ )
        {
            if ( symlink( PACKAGE_NAME, pt->program ) < 0 )
            {
                printf( "symlink %s -> %s failed (%s)\n",
                        PACKAGE_NAME, pt->program,
                        strerror( errno ));
            }
        }

        return 0;
    }

    goto usage;
}

int
main( int argc, char ** argv )
{
    struct prog_table * pt;
    char * prog;
    int doerr = 1;

    argv_zero = argv[0];

    if ((prog = rindex( argv[0], '/' )) != NULL)
        prog++;
    else
        prog = argv[0];

    for ( pt = prog_table; pt->program != NULL; pt++ )
    {
        if ( strcmp( prog, pt->program ) == 0 )
        {
            if ( pt->priv == 0 )
                remove_privs();
            return pt->mainfunc( argc, argv );
        }
    }

    remove_privs();

    if ( strncmp( prog, PACKAGE_NAME, strlen(PACKAGE_NAME) ) == 0 )
    {
        int r = handle_admin( argc-1, argv+1 );
        if ( r != -2 )
            return r;
        /* fall through */
        doerr = 0;
    }

    if ( doerr == 1 )
        printf( "\nunknown program '%s'; known programs:\n",
                prog );
    else
        printf( "\nknown programs:\n" );

    printf( "\n    " );

    for ( pt = prog_table; pt->program != NULL; pt++ )
    {
        const char * p;
        const char * h;

        p = pt->program;
        h = pt->help;

        printf( "%12s - %s\n    ", p, h );
    }

    printf( "\n\n" );

    return 0;

}

#if defined(sparc)
int
arc4random( void )
{
    static int arc4randinitted = 0;
    if ( arc4randinitted == 0 )
    {
        srandom( time( NULL ) * getpid() );
        arc4randinitted = 1;
    }
    return random();
}
#endif
