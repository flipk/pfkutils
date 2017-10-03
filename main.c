
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>

#include "main_prog.h"
#include "version.h"

#if defined(sparc)
#include <strings.h>
#endif

static char * argv_zero;

struct prog_table {
    char * program;
    int (*mainfunc)( int argc, char ** argv );
    int priv;
    char * help;
};

#define PROG(priv,name,help) extern int name##_main( int argc, char ** argv );
#include "main.h"
#undef PROG

static struct prog_table prog_table[] = {

#define PROG(priv,name,help) { #name, name##_main, priv, #help },
#include "main.h"
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

        if ( strcmp( p, ADMINNAME ) == 0 )
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
    printf("build date: %s\n", BUILD_DATE);
    printf("version: %s\n", PKUTILS_VERSION);
    if ( argc == 0 || 
         strcmp( argv[0], "-help" ) == 0 )
    {
    usage:
        printf( "usage: %s [-dellinks] [-makelinks] "
                "[-help]\n", ADMINNAME );
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

        if ( stat( ADMINNAME, &sb ) < 0 )
        {
            printf( "this program must be run from the "
                    "directory where links to be installed\n" );
            return -1;
        }

        unlink_all_links();

        for ( pt = prog_table; pt->program != NULL; pt++ )
        {
            if ( symlink( ADMINNAME, pt->program ) < 0 )
            {
                printf( "symlink %s -> %s failed (%s)\n",
                        ADMINNAME, pt->program,
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

    if ( strcmp( prog, ADMINNAME ) == 0 )
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
        char * p;
        char * h;

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
