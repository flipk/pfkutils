
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
