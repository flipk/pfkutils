
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
