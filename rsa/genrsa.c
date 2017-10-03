/*
    This file is part of the "pkutils" tools written by Phil Knaack
    (pknaack1@netscape.net).
    Copyright (C) 2008  Phillip F Knaack

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#if 0
set -e -x
gcc -O3 genrsa.c timing.c keyfile.c gmp-4.1.2/.libs/libgmp.a -o g
strip g
exit 0
#endif

#include <stdio.h>
#include <errno.h>
#include "gmp.h"
#include "timing.h"
#include "keyfile.h"

static gmp_randstate_t   random_state;

void
makeprime( mpz_t out, int bits )
{
    mpz_urandomb( out, random_state, bits );
    mpz_setbit( out, bits-1 );
    mpz_nextprime( out, out );
}

int
makersa( mpz_t n, mpz_t d, mpz_t e, int bits )
{
    mpz_t p, q, n2, temp;
    int done = 0;
    int reps = 0;

    mpz_init( temp );
    mpz_init( p );
    mpz_init( q );
    mpz_init( n2 );
    gmp_randinit_default( random_state );
    gmp_randseed_ui( random_state, arc4random() );

/*
  make 2 random prime numbers, p and q
  n = pq
  n2 = (p-1)(q-1)
  randomly choose value 'e', which must be relatively prime to n2.
  compute a 'd' so that:
  ed =~ 1 mod n2
  note this can be calculated by using the modulo inverse function:
  d = e^-1 mod n2.
  also note that gmp provides a function to calculate the inverse
  which internally uses the extended euclidian gcd algorithm.
  message m is encrypted to cyphertext c:  c = m^e mod n
  cyphertext c is decrypted to message m:  m = c^d mod n
  throw away p and q
  public key: (e,n)
  private key: (d,n)
*/

    do {

        reps++;

        makeprime( p, bits );
        makeprime( q, bits );

        mpz_mul( n, p, q );

        mpz_sub_ui( p, p, 1 );
        mpz_sub_ui( q, q, 1 );
        mpz_mul( n2, p, q );

        makeprime( d, bits * 15 / 8 );

        /* verify that d is relatively prime to n2 */

        mpz_gcd( temp, d, n2 );

        if ( mpz_cmp_ui( temp, 1 ) != 0 )
            continue;

        /* calculate the inverse mod n2 of d */

        if ( mpz_invert( temp, d, n2 ) == 0 )
            continue;

        /* now calculate d */

        mpz_mod( e, temp, n2 );

        /* test d and e, note this
           reuses the variables p and q
           as temp values */

        /* make a random message to encrypt */
        mpz_urandomb( temp, random_state, bits-4 );

        /* encrypt temp into p */
        mpz_powm( p, temp, e, n );

        /* decrypt p into q */
        mpz_powm( q, p, d, n );

        /* ensure original message equals decrypted message */
        if ( mpz_cmp( temp, q ) == 0 )
            done = 1;

    } while ( done == 0 );

    mpz_clear( temp );
    mpz_clear( p );
    mpz_clear( q );
    mpz_clear( n2 );
    gmp_randclear( random_state );

    return reps;
}

int
main( int argc, char ** argv )
{
    mpz_t    n, d, e;
    char   * out;
    int      t, r, bits;
    TS_VAR   ts;

    if ( argc != 2 )
    {
    usage:
        printf( "usage: g bits, where  32<=bits<=2048\n" );
        return 1;
    }

    bits = atoi( argv[1] );
    if ( bits < 32 || bits > 2048 )
        goto usage;

    mpz_init( n );
    mpz_init( d );
    mpz_init( e );

    TS_START(ts);
    r = makersa( n, d, e, bits );
    t = TS_END(ts);

    printf( "makersa time = %d ms\n", t );
    printf( "reps = %d\n", r );

    if ( putkeys( n, d, e, (bits*2/8)-1 ) < 0 )
        printf( "error writing keys to disk: %s\n", strerror(errno) );

    mpz_clear( n );
    mpz_clear( d );
    mpz_clear( e );

    return 0;
}
