/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

#include "d3encdec.H"

#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>

extern "C" int
etg_genkey_main( int argc, char ** argv )
{
    srandom( time(NULL) * getpid() );

    if ( d3des_crypt_genkey() == false )
    {
        fprintf( stderr, "key generation failed\n" );
        return 1;
    }
    return 0;
}
