
#include <stdio.h>
#include "main.h"

char *
viewtags_input_tag( void )
{
    static char tag[ 64 ];
    printf( "tag? " );
    fflush( stdout );
    if ( scanf( "%63s", &tag ) == EOF )
        return NULL;
    return tag;
}
