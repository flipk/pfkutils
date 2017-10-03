/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_PIDS 15

int
waitpid_main( int argc, char ** argv )
{
    int pids[ MAX_PIDS ];
    int numpids, pidsleft;
    int i;

    if ( argc > MAX_PIDS )
    {
        printf( "too many pids, max %d\n", MAX_PIDS );
        return 1;
    }

    numpids = 0;
    for ( i = 0; i < MAX_PIDS; i++ )
        pids[i] = -1;

    for ( argc--, argv++; argc-- > 0; numpids++, argv++ )
    {
        pids[numpids] = atoi( argv[0] );
        if ( pids[numpids] ==  0 )
        {
            printf( "is arg '%s' not a pid?\n", argv[0] );
            return 1;
        }
    }

    printf( "waiting for %d pids: ", numpids );
    for ( i = 0; i < numpids; i++ )
        printf( "%d ", pids[i] );
    printf( "\n" );

    pidsleft = numpids;
    while ( pidsleft > 0 )
    {
        sleep( 2 );
        for ( i = 0; i < numpids; i++ )
        {
            if ( pids[i] == -1 )
                continue;

            if (( kill( pids[i], 0 ) < 0 ) && 
                ( errno == ESRCH ))
            {
                printf( "waitpid: pid %d died, %d left\n", 
                        pids[i], --pidsleft );
                pids[i] = -1;
            }
        }
    }

    printf( "waitpid: all named pids have exited\n" );

    return 0;
}
