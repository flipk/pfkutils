/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/

#include "pfkutils_config.h"

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

#include <string>
#include <iostream>
#include <iomanip>

// autogenerated by Makefile.pfk and placed in
// $(OBJDIR)/pfkutils-config.c
extern const char pfkutils_config[];

using namespace std;

struct prog_table {
    string  program;
    int (*mainfunc)( int argc, char ** argv );
    int priv;
    string  help;
};

#define PROG(priv,name,help) \
    extern "C" int name##_main( int argc, char ** argv );
#include "programs.h"
#undef PROG

static struct prog_table prog_table[] = {

#define PROG(priv,name,help) { #name, &name##_main, priv, help },
#include "programs.h"
#undef PROG

    { "", NULL, 0, "" }
};

static void
unlink_all_links( void )
{
    DIR * d;
    struct dirent * de;
    char p[ 1024 ];
    int count;

    d = opendir( "." );
    if ( d == NULL )
    {
        printf( "opendir of '.' failed (%s)\n", strerror( errno ));
        exit( 1 );
    }

    while (( de = readdir( d )) != NULL )
    {
        count = readlink( de->d_name, p, sizeof(p)-1 );
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
    if (seteuid( getuid() ) < 0)
        fprintf(stderr, "failure removing priviledges!\n");
}

static int
handle_admin( int argc, char ** argv )
{
    string arg0(argc > 0 ? argv[0] : "");

    printf("URL: %s\n", GIT_SRC_URL);
    printf("version: %s %s\n", PACKAGE_STRING, BUILD_DATE);
    printf("commit: %s %s\n", PFKUTILS_BRANCH, PFKUTILS_COMMIT);
    if ( argc == 0 || arg0 == "-help" )
    {
    usage:
        printf( "usage: pfkutils [-dellinks] [-makelinks] "
                "[-config] [-help]\n" );
        printf("   -dellinks : remove all symlinks pointing to pfkutils\n");
        printf("   -makelinks : make symlinks for all known tools\n");
        printf("   -config : print out build-time configuration\n");
        printf("   -help : print help (this message)\n");
        printf( "or symlink 'pfkutils' to one of the following names\n");
        return -2;
    }

    if ( arg0 == "-dellinks" )
    {
        unlink_all_links();
        return 0;
    }

    if ( arg0 == "-makelinks" )
    {
        unlink_all_links();

        for ( struct prog_table * pt = prog_table;
              pt->mainfunc != NULL;
              pt++ )
        {
            if ( symlink( PACKAGE_NAME, pt->program.c_str() ) < 0 )
            {
                cout << "symlink " << PACKAGE_NAME
                     << " -> " << pt->program 
                     << " failed (" << strerror( errno )
                     << ")" << endl;
            }
        }

        return 0;
    }

    if ( arg0 == "-config" )
    {
        cout << pfkutils_config;
        return 0;
    }

    goto usage;
}

int
main( int argc, char ** argv )
{
    string arg0(argv[0]);
    string prog;
    struct prog_table * pt;
    int doerr = 1;
    size_t slashpos;

    slashpos = arg0.find_last_of('/');
    if (slashpos == string::npos)
        prog = arg0;
    else
        prog = arg0.substr(slashpos+1);

    // pfksh may be invoked as a login shell, in which case
    // argv[0] actually starts with a dash '-'.

    for ( pt = prog_table; pt->mainfunc != NULL; pt++ )
    {
        if ( prog == pt->program ||
             ( prog[0] == '-' && 
               prog.compare(1, string::npos,
                            pt->program) == 0 ))
        {
            if ( pt->priv == 0 )
                remove_privs();
            return pt->mainfunc( argc, argv );
        }
    }

    remove_privs();

    if ( prog == PACKAGE_NAME )
    {
        int r = handle_admin( argc-1, argv+1 );
        if ( r != -2 )
            return r;
        /* fall through */
        doerr = 0;
    }

    if ( doerr == 1 )
        cout << endl << "unknown program '"
             << prog << "'; known programs:" << endl;
    else
        cout << endl << "known programs:" << endl;

    for ( pt = prog_table; pt->mainfunc != NULL; pt++ )
    {
        cout << setw(18) << pt->program
             << " - " << pt->help << endl;
    }

    return 0;
}
