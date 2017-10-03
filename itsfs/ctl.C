/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

#if 0
g++ ctl.C -o itsfsctl -Ditsfsctl_main=main
exit 0
#endif

#include "mytypes.h"
#include "control_pipe.H"

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>

#define BAIL(x)  do { printf x; return 1; } while ( 0 )

int
itsfsctl_main( int argc, char ** argv )
{
    int p;

    if ( argc == 1 )
    {
        // xxx print usage
        return 1;
    }
    else if ( strcmp( argv[1], "stop" ) == 0 )
    {
        int serverpid;
        FILE * statusfile;
        char pidline[80];

        statusfile = fopen( "/i/status", "r" );
        if ( statusfile == NULL )
            BAIL(( "unable to open status file\n" ));
        if ( fgets( pidline, 80, statusfile ) != pidline )
            BAIL(( "error reading statusfile\n" ));
        if ( sscanf( pidline, "PID: %d", &serverpid ) != 1 )
            BAIL(( "error scanning status file pidline\n" ));
        fclose( statusfile );

        printf( "server is pid %d\n", serverpid );
        // kill server
        p = open( "/i/command", O_WRONLY );
        if ( p < 0 )
            BAIL(( "unable to open command file\n" ));
        write( p, "exit\r", 5 );
        close( p );

        int counter = 5;
        while ( --counter > 0 )
        {
            sleep( 1 );
            if ( kill( serverpid, 0 ) < 0 )
                break;
        }
        if ( counter <= 0 )
            BAIL(( "error: server failed to die!\n" ));
        printf( "server is stopped.\n" );
        return 0;
    }
    else if ( strcmp( argv[1], "status" ) == 0 )
    {
        return system( "cat /i/status" );
    }
}
