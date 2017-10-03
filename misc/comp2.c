#if 0
set -e -x
gcc -DCOMP=1 pk.c -o pkcomp  -Lzlib-1.2.1/ -lz -g3
gcc -DCOMP=0 pk.c -o pkuncomp -Lzlib-1.2.1/ -lz -g3
exit 0
#endif

/* reads from standard input (fd 0) and writes to standard output (fd 1) */

#include <stdio.h>
#include "zlib-1.2.1/zlib.h"

int
main()
{
    z_stream zs;
    char buf[ 5000 ];
    char out[ 5000 ];
    int rc;

    zs.zalloc = 0;
    zs.zfree = 0;
    zs.opaque = 0;

#if COMP
    deflateInit( &zs, Z_DEFAULT_COMPRESSION );
#else
    inflateInit( &zs );
#endif

    while ( 1 )
    {
        int cc = read( 0, buf, 5000 );
        if ( cc < 0 )
            fprintf( stderr, "error %d in read\n", cc);
        if ( cc == 0 )
            break;

        zs.next_in   = buf;
        zs.avail_in  = cc;
        zs.next_out  = out;
        zs.avail_out = 5000;

        while ( zs.avail_in > 0 )
        {
#if COMP
            deflate( &zs, Z_NO_FLUSH );
#else
            inflate( &zs, Z_NO_FLUSH );
#endif
            if ( zs.avail_out != 5000 )
            {
                write( 1, out, 5000 - zs.avail_out );
                zs.next_out = out;
                zs.avail_out = 5000;
            }
        }
    }

    zs.next_in = buf;
    zs.avail_in = 0;
    zs.next_out = out;
    zs.avail_out = 5000;

    while ( 1 )
    {
#if COMP
        rc = deflate( &zs, Z_FINISH );
#else
        rc = inflate( &zs, Z_FINISH );
#endif
        if ( zs.avail_out != 5000 )
        {
            write( 1, out, 5000 - zs.avail_out );
            zs.next_out = out;
            zs.avail_out = 5000;
        }
        if ( rc == Z_STREAM_END )
            break;
        if ( rc == Z_BUF_ERROR )
        {
            fprintf( stderr, "Z_BUF_ERROR while finishing!\n" );
            exit(1);
        }
    }

#if COMP
    rc = deflateEnd( &zs );
#else
    rc = inflateEnd( &zs );
#endif
    if ( rc != Z_OK )
        fprintf( stderr, "returned %d at exit!\n", rc );

    return 0;
}
