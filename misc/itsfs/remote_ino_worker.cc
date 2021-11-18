
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <setjmp.h>
#include <signal.h>
#include "remote_ino.H"
#include "config.h"

extern "C" {
    int inet_aton( char *, struct in_addr * );
    int itsfsriw_main( int argc, char ** argv );
};

jmp_buf signal_buffer;

void
sighand( int s )
{
    longjmp( signal_buffer, -1 );
}

int
itsfsriw_main( int argc, char ** argv )
{
    bool verbose = false;
    bool symlinks = true;
    bool dirsymlinks = true;
    bool checkparent = true;

    char * server = getenv( "ITSFS_SERVER_IPADDR" );
    if ( !server )
    {
        printf( "please setenv ITSFS_SERVER_IPADDR\n" );
        return -1;
    }

    if ( argc == 1 )
    {
    usage:
        printf( "usage: itsfsriw [-nolinks -nodirlinks -verbose -ncp] <fsname>\n" );
        return -1;
    }

    argc--;
    argv++;

    while ( argc > 1 )
    {
        if ( argv[0][0] != '-' )
            goto usage;
        if ( strcmp( argv[0], "-nolinks" ) == 0 )
            symlinks = false;
        else if ( strcmp( argv[0], "-nodirlinks" ) == 0 )
            dirsymlinks = false;
        else if ( strcmp( argv[0], "-verbose" ) == 0 )
            verbose = true;
        else if ( strcmp( argv[0], "-ncp" ) == 0 )
            checkparent = false;
        else
            goto usage;
        argc--;
        argv++;
    }

    if ( argv[0][0] == '-' )
        goto usage;

    struct in_addr ad;
    if ( !inet_aton( server, &ad ))
    {
        printf( "inet bailed\n" );
        return -1;
    }

    remote_inode_server_tcp svr( htonl( ad.s_addr ),
                                 SLAVE_PORT,
                                 (uchar*)argv[0],
                                 verbose, symlinks, dirsymlinks );
    char logpath[ 200 ];
    FILE * fd;
    char * home = getenv( "HOME" );
    char * host = getenv( "HOST" );
    char * root = getenv( "CLEARCASE_ROOT" );
    char * pwd  = getenv( "PWD" );

    if ( !home ) home = "nohome";
    if ( !host ) host = "unknownhost";
    if ( !root ) root = "";
    if ( !pwd  ) pwd  = "unknownpwd";

    sprintf( logpath, "%s/.y.itsfsriw.%s.%d", home, host, getpid() );
    fd = fopen( logpath, "we" );
    if ( fd )
    {
        fprintf( fd, "itsfsriw,%s,%d,p%d,%s,%s\n",
                 host, getpid(), getppid(), root, pwd );
        fclose( fd );
    }
    signal( SIGINT,  sighand );
    signal( SIGHUP,  sighand );
    signal( SIGTERM, sighand );
    if ( setjmp( signal_buffer ) == 0 )
        svr.dispatch_loop( checkparent );
    unlink( logpath );

    return 0;
}
