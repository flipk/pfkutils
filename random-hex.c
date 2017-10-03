#include <stdio.h>
#include <stdlib.h>

int
random_hex_main( int argc, char ** argv )
{
    int num;

    srandom( getpid() * time( NULL ));
    if ( argc != 2 )
        exit( 1 );
    num = atoi( argv[1] );
    while ( num-- > 0 )
        putchar( "0123456789abcdef"[random()%16] );
    putchar( '\n' );
    return 0;
}
