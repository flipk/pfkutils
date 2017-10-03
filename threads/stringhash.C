
#include "stringhash.H"

// #include <stdio.h>

int
string_hash( char * string )
{
    int ret = 5, mult = 1; char * p = string;
    for ( ; *p; p++ )
    {
        ret += ( (int)(*p) * mult );
        mult++;
    }
//    fprintf( stderr, "string '%s' hashes to key %d\n", string, ret );
    return ret;
}
