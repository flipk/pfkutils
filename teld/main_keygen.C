/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

#include <stdio.h>
#include <stdlib.h>

#include "keys.H"

// todo : put key file name on cmdline

extern "C" int
pfktel_keygen_main()
{
    PfkKeyPairs * pairs;

    pairs = new PfkKeyPairs( 0x10000 );
    delete pairs;

    return 0;
}
