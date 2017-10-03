/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

#include "md5.h"
#include <stdio.h>

int
main()
{
    MD5_CTX          ctx;
    unsigned char    digest[16];
    unsigned char    inbuf[ 1024 ];
    unsigned int     len;
    FILE           * f;

    MD5Init( &ctx );

    f = fopen( "md5.c", "r" );

    while ( 1 )
    {
        len = fread( inbuf, 1, sizeof(inbuf), f );
        if ( len == 0 )
            break;
        MD5Update( &ctx, inbuf, len );
    }

    MD5Final( digest, &ctx );

    printf( "digest: " );
    for ( len = 0; len < sizeof(digest); len++ )
    {
        printf( "%02x", digest[len] );
    }
    printf( "\n" );

    return 0;
}
