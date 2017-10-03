#if 0
set -e -x
gcc -O3 decrypt.c timing.c keyfile.c gmp-4.1.2/.libs/libgmp.a -o d
strip d
exit 0
#endif

#include <stdio.h>
#include "gmp.h"
#include "keyfile.h"
#include "timing.h"

#define VERBOSE 0

// file layout:
//   each block preceded by one byte marker
//   marker   contents
//     b       bytes per block
//     p       block data
//     e       bytes in last block

int
cvthex1( char dig )
{
    if ( dig >= '0' && dig <= '9' )
        return dig-'0';
    if ( dig >= 'a' && dig <= 'f' )
        return dig-'a'+10;
    return 0;
}

int
cvthex2( char * hex )
{
    return (cvthex1(hex[0]) << 4) + cvthex1(hex[1]);
}

int
main( int argc, char ** argv )
{
    mpz_t   n, d, m, c;
    int     bytes, i, cc, finish;
    char  * buffer;
    char  * bufferx;
    TS_VAR  ts;

    mpz_init( n );
    mpz_init( d );
    mpz_init( m );
    mpz_init( c );

    if ( getkeys_priv( n, d, &bytes ) != 0 )
        return 1;

    buffer  = (char*) malloc( bytes     );
    bufferx = (char*) malloc( bytes*2+2 );

#if VERBOSE
    mpz_get_str( bufferx, 16, d );
    fprintf( stderr, "d = %s\n", bufferx );
    mpz_get_str( bufferx, 16, n );
    fprintf( stderr, "n = %s\n", bufferx );
#endif

    TS_START(ts);

    // m is message
    // c is cipher

    finish = 0;
    
    while ( 1 )
    {
        int ch = fgetc( stdin );
        int tmp;

        if ( ch == EOF )
            break;

        switch ( ch )
        {
        case 'b':
            if ( fgets( buffer, bytes, stdin ) == NULL )
                return 2;
            tmp = atoi( buffer );
            if ( tmp != bytes )
            {
                fprintf( stderr, "bytes mismatch! %d != %d\n", 
                         tmp, bytes );
                return 3;
            }
            break;

        case 'e':
            if ( fgets( buffer, bytes, stdin ) == NULL )
                return 4;
            finish = atoi( buffer );
            break;

        case 'p':
            mpz_inp_raw( c, stdin );

#if VERBOSE
            mpz_get_str( bufferx, 16, c );
            fprintf( stderr, "c = %s\n", bufferx );
#endif

            // decrypt c to m
            mpz_powm( m, c, d, n );

            mpz_get_str( bufferx, 16, m );
#if VERBOSE
            fprintf( stderr, "m = %s\n", bufferx );
#endif

            cc = strlen( bufferx );
            i = ((finish?finish:bytes)*2) - cc;

#if VERBOSE
            fprintf( stderr, "strlen diff = %d\n", i );
#endif
            if ( i != 0 )
            {
                memmove( bufferx+i, bufferx, cc+1 );
                memset( bufferx, '0', i );
            }

            // output plaintext
            for ( i = 0; i < (finish?finish:bytes); i++ )
                buffer[i] = cvthex2( bufferx + i*2 );

            write( 1, buffer, finish?finish:bytes );
            break;

        default:
            return 5;
        }
    }

    fprintf( stderr, "decrypt time = %d ms\n", TS_END(ts) );

    free( buffer  );
    free( bufferx );
    mpz_clear( n );
    mpz_clear( d );
    mpz_clear( m );
    mpz_clear( c );
    return 0;
}
