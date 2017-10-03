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

#include <stdio.h>
#include <errno.h>
#include "gmp.h"

#define PUBFILE  "rsa.pub"
#define PRIVFILE "rsa.priv"

/*
  public key: (e,n)
  private key: (d,n)

  mpz_set_str( mpz_t, char *, int base ) returns 0 if string valid 
  mpz_get_str( char *, int base, mpz_t ) will malloc if inp is null
  mpz_sizeinbase( mpz_t, int base ) + 2 is size required for string
*/

int
putkeys( mpz_t n, mpz_t d, mpz_t e, int bytes )
{
    FILE   * f;

    if ( unlink( PUBFILE ) < 0 )
        if ( errno != ENOENT )
            return -1;

    f = fopen( PUBFILE, "w" );
    if ( f == NULL )
        return -1;

    fprintf( f, "b=%d\ne=", bytes );
    mpz_out_str( f, 36, e );
    fputs( "\nn=", f );
    mpz_out_str( f, 36, n );
    fputc( '\n', f );
    fclose( f );
    chmod( PUBFILE, 0444 );

    if ( unlink( PRIVFILE ) < 0 )
        if ( errno != ENOENT )
            return -1;

    f = fopen( PRIVFILE, "w" );
    if ( f == NULL )
        return -1;

    fprintf( f, "b=%d\nd=", bytes );
    mpz_out_str( f, 36, d );
    fputs( "\nn=", f );
    mpz_out_str( f, 36, n );
    fputc( '\n', f );
    fclose( f );
    chmod( PRIVFILE, 0400 );

    return 0;
}

#define MAXLINE 1000

/* return 0 if read ok */
int
getkeys_pub ( mpz_t n, mpz_t e, int *bytes )
{
    char line[MAXLINE];
    FILE * f;
    int ret = -1;

    f = fopen( PUBFILE, "r" );
    if ( f == NULL )
        return -1;

    if ( fgets( line, MAXLINE, f ) == NULL )
        goto bail;
    if ( strncmp( line, "b=", 2 ) != 0 )
        goto bail;

    *bytes = atoi( line+2 );

    if ( fgets( line, MAXLINE, f ) == NULL )
        goto bail;
    if ( strncmp( line, "e=", 2 ) != 0 )
        goto bail;
    if ( mpz_set_str( e, line+2, 36 ) != 0 )
        goto bail;

    if ( fgets( line, MAXLINE, f ) == NULL )
        goto bail;
    if ( strncmp( line, "n=", 2 ) != 0 )
        goto bail;
    if ( mpz_set_str( n, line+2, 36 ) != 0 )
        goto bail;

    ret = 0;

 bail:
    fclose( f );
    return ret;
}

/* return 0 if read ok */
int
getkeys_priv( mpz_t n, mpz_t d, int *bytes )
{
    char line[MAXLINE];
    FILE * f;
    int ret = -1;

    f = fopen( PRIVFILE, "r" );
    if ( f == NULL )
        return -1;

    if ( fgets( line, MAXLINE, f ) == NULL )
        goto bail;
    if ( strncmp( line, "b=", 2 ) != 0 )
        goto bail;

    *bytes = atoi( line+2 );

    if ( fgets( line, MAXLINE, f ) == NULL )
        goto bail;
    if ( strncmp( line, "d=", 2 ) != 0 )
        goto bail;
    if ( mpz_set_str( d, line+2, 36 ) != 0 )
        goto bail;

    if ( fgets( line, MAXLINE, f ) == NULL )
        goto bail;
    if ( strncmp( line, "n=", 2 ) != 0 )
        goto bail;
    if ( mpz_set_str( n, line+2, 36 ) != 0 )
        goto bail;

    ret = 0;

 bail:
    fclose( f );
    return ret;
}
