
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
