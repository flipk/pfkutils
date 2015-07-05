
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
