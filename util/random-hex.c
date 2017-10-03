
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>

static int
random_main( int hex, int argc, char ** argv )
{
    int num;

    srandom( getpid() * time( NULL ));
    if ( argc != 2 )
        exit( 1 );
    num = atoi( argv[1] );
    while ( num-- > 0 )
    {
        if (hex)
            putchar( "0123456789abcdef"[random()%16] );
        else
            putchar(
 "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
             [random()%62] );
    }
    putchar( '\n' );
    return 0;
}

int
random_text_main( int argc, char ** argv )
{
    return random_main( 0, argc, argv );
}

int
random_hex_main( int argc, char ** argv )
{
    return random_main( 1, argc, argv );
}
