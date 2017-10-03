/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#include "test_hash.H"

int
main()
{
    thing_list  list;
    thing_hash  hash;

    srandom( time(0) * getpid() );

    add_entries( &hash, &list );
    del_entries( &hash, &list );

    return 0;
}
