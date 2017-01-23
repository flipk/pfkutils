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
