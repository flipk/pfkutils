
#include <sys/types.h>
#include <stdio.h>
#include <dirent.h>
#include <strings.h>
#include <md5.h>
#include <signal.h>
#include <errno.h>

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

static void
status( int s )
{
    if ( current_file )
    {
        fprintf( stderr,
                 "current_file=\"%s\"\n", current_file );
    }
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

        if (( de->d_type & DT_DIR ) &&
            (( strcmp( ent, "."  ) == 0 ) ||
             ( strcmp( ent, ".." ) == 0 )))
        {
            continue;
        }

        s = NEW( char, strlen(ent)+strlen(name)+2 );
        sprintf( s, "%s/%s", name, ent );
        current_file = s;

        switch ( de->d_type )
        {
        case DT_DIR:
            recurse( s );
            break;

        case DT_LNK:
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

        case DT_REG:
            MD5File( s, buf );
            printf( "F %s\nH %s %s\n", s, s, buf );
            break;

        default:
            printf( "U %s\n", s );
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

    signal( SIGINFO, status );

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
