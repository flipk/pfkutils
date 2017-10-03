/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

#include <stdio.h>
#include "main.h"

char *
viewtags_input_tag( void )
{
    static char tag[ 64 ];
    printf( "tag? " );
    fflush( stdout );
    if ( scanf( "%63s", tag ) == EOF )
        return NULL;
    return tag;
}
