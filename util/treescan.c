/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

#include <sys/types.h>
#include <stdio.h>
#include <dirent.h>
#include <strings.h>
#include "pk-md5.h"
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>

#define NEW(x,y)  (x *)calloc(y, sizeof(x))
#define FREE(x)  { if (x != NULL) free(x); x = NULL; }

static char *current_file = NULL;

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

#if !CYGWIN
static void
status( int s )
{
    if ( current_file )
    {
        fprintf( stderr,
                 "current_file=\"%s\"\n", current_file );
    }
}
#endif

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

#if CYGWIN
        /* cygwin does not provide a 'type' field in dirents,
           so we have to stat to know what they are. */
        struct stat sb;
#endif

        s = NEW( char, strlen(ent)+strlen(name)+2 );
        sprintf( s, "%s/%s", name, ent );

#if CYGWIN
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
#else
        if ( de->d_type == DT_DIR )
            type = TYPE_DIR;
        else if ( de->d_type == DT_LNK )
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

        current_file = s;

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

#if !CYGWIN
    signal( SIGINFO, status );
#endif

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
