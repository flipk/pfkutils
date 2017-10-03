
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

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>

#include "dll2.H"

class rmstar_item {
    void * operator new( size_t s, int itemlen ) {
        char * ret = new char[itemlen + 1 + sizeof(rmstar_item)];
        return (void*)ret;
    }
    rmstar_item( char *data ) {
        strcpy( item, data );
    }
public:
    LListLinks<rmstar_item> links[1];
    char item[0];
    static rmstar_item * new_item_va( const char * format, va_list ap )
        __attribute__ (( format( printf, 1, 0 )));
    static rmstar_item * new_item( const char * format, ... )
        __attribute__ (( format( printf, 1, 2 )));
    void operator delete( void * ptr ) {
        char * p = (char*) ptr;
        delete[] p;
    }
};

// static
rmstar_item *
rmstar_item :: new_item_va( const char * format, va_list ap )
{
    static char data[2048];
    vsnprintf( data, 2047, format, ap );
    int l = strlen( data );
    data[2047] = 0;
    return new(l) rmstar_item(data);
}

// static
rmstar_item * 
rmstar_item :: new_item( const char * format, ... )
{
    va_list ap;
    va_start( ap, format );
    rmstar_item * ret = new_item_va( format, ap );
    va_end( ap );
    return ret;
}

typedef LList<rmstar_item,0> RMSTAR_LIST;

static RMSTAR_LIST log;

void logit( const char * format, ... )
    __attribute__ (( format( printf, 1, 2 )));

void
logit( const char * format, ... )
{
    va_list ap;
    va_start( ap, format );
    log.add( rmstar_item::new_item_va( format, ap ));
    va_end( ap );
}

static int
my_unlink( bool isdir, char * path )
{
    int r;

    r = isdir ? rmdir( path ) : unlink( path );

#ifdef FREEBSD
    if ( r == 0 )
        return 0;
    if ( errno == EPERM )
    {
        r = chflags( path, 0 );
        if ( r == 0 )
            r = isdir ? rmdir( path ) : unlink( path );
    }
#endif
    return r;
}

/*

rmthings <list L>
   foreach i in L
      if i is !dir
     unlink it
      else
     list dirs;
     while elem=readdir
        if elem is dir
           add to dirs list
        else
           unlink elem
     rmthings dirs
     rmdir i

*/

static void
rmthings( RMSTAR_LIST * l )
{
    char * path;
    struct stat sb;

    rmstar_item * i;

    while ((i = l->get_head()) != NULL)
    {
        l->remove( i );
        path = i->item;

        if ( lstat( path, &sb ) < 0 )
        {
            logit( "stat %s: %s\n",
                   path, strerror( errno ));
            delete i;
            continue;
        }

        if ( !S_ISDIR( sb.st_mode ))
        {
            if ( my_unlink( false, path ) < 0 )
                logit( "unlink %s: %s\n",
                       path, strerror( errno ));
            delete i;
            continue;
        }

        DIR * d = opendir( path );
        if ( d == NULL )
        {
            logit( "opendir %s: %s\n",
                   path, strerror( errno ));
            delete i;
            continue;
        }

        RMSTAR_LIST dirs;
        struct dirent * de;
        while (( de = readdir( d )) != NULL )
        {
            if (( strcmp( de->d_name, "." ) == 0 ) ||
                ( strcmp( de->d_name, ".." ) == 0 ))
            {
                continue;
            }

            if ( S_ISDIR( sb.st_mode ))
            {
                rmstar_item * newi;
                newi = rmstar_item::new_item( "%s/%s", path, de->d_name );
                dirs.add( newi );
            }
            else
            {
                if ( my_unlink( false, de->d_name ) < 0 )
                    logit( "unlink %s: %s\n",
                           path, strerror( errno ));
            }
        }

        closedir( d );
        rmthings( &dirs );
        printf( "%s\n", path );

        if ( my_unlink( true, path ) < 0 )
            logit( "rmdir %s: %s\n",
                   path, strerror( errno ));

        delete i;
    }
}

extern "C" {
    int rmstar_main( int, char ** );
}

int
rmstar_main( int argc, char ** argv )
{
    RMSTAR_LIST l;

    argc--; argv++;
    while ( argc-- > 0 )
    {
        rmstar_item * newi;
        newi = rmstar_item::new_item( "%s", argv[0] );
        l.add( newi );
        argv++;
    }

    if ( l.get_cnt() > 0 )
        rmthings( &l );

    /* handle displaying of logs */

    if ( log.get_cnt() > 0 )
    {
        rmstar_item * i;

        printf( "there were %d errors; view them? (y,n) ",
                log.get_cnt() );
        fflush( stdout );

        char buf[80];
        buf[0] = 0;
        if ( fgets( buf, 79, stdin ) && buf[0] == 'y' )
        {
            while ( (i = log.get_head()) != NULL )
            {
                log.remove( i );
                printf( "%s", i->item );
                delete i;
            }
        }
    }

    return 0;
}
