/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

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
