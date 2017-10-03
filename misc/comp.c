/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

/*
 * reference implementation for compressor/decompressor
 * that includes periodic sync/flushing for an interactive
 * data stream.
 */

#include <sys/time.h>
#include <sys/select.h>
#include <stdio.h>
#include "zlib-1.1.4/zlib.h"

#define BUF 8192

static z_stream zs;
static int (*func_move)( z_streamp, int );
static int (*func_end) ( z_streamp );

static void
consume_zdata( int flush )
{
    char obuf[ BUF ];
    int cc;

 again:
    zs.next_out = obuf;
    zs.avail_out = BUF;

    cc = func_move( &zs, flush );

    /* z-buf-error is expected in flush cases because
       there may actually be nothing to flush. */

    if ( cc < 0 && cc != Z_BUF_ERROR )
    {
        fprintf( stderr, "error %d\n", cc );
        exit( 1 );
    }

    if ( zs.avail_out != BUF )
        write( 1, obuf, BUF - zs.avail_out );

    if ( zs.avail_out == 0 )
        goto again;
}

int
main( int argc, char ** argv )
{
    char ibuf[ BUF ];
    int synced = 1;

    if ( argc != 2 )
        return 1;

    zs.zalloc = Z_NULL;
    zs.zfree = Z_NULL;
    zs.opaque = 0;

    if ( strcmp( argv[1], "c" ) == 0 )
    {
        deflateInit( &zs, Z_BEST_COMPRESSION );
        func_move = deflate;
        func_end  = deflateEnd;
    }
    else if ( strcmp( argv[1], "u" ) == 0 )
    {
        inflateInit( &zs );
        func_move = inflate;
        func_end  = inflateEnd;
    }
    else
        return 2;

    while ( 1 )
    {
        int cc;
        fd_set fds;

        do {

            struct timeval tv = { 0, 250000 };
            FD_ZERO( &fds );
            FD_SET( 0, &fds );

            if ( !synced )
                cc = select( 1, &fds, 0, 0, &tv );
            else
                cc = select( 1, &fds, 0, 0, NULL );

            if ( cc == 0 )
            {
                if ( func_move == deflate )
                    consume_zdata( Z_SYNC_FLUSH );
                synced = 1;
            }

        } while ( cc == 0 );

        cc = read( 0, ibuf, BUF );
        if ( cc <= 0 )
            break;

        synced = 0;
        zs.next_in = ibuf;
        zs.avail_in = cc;

        consume_zdata( Z_NO_FLUSH );
    }

    consume_zdata( Z_FINISH );
    func_end( &zs );

    return 0;
}
