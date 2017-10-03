#if 0
g++ -g3 -c test_hash.C
g++ -g3 -c test_hash_add.C
g++ -g3 -c test_hash_del.C
g++ -g3 -c dll2_hash.C
g++ test_hash.o test_hash_add.o test_hash_del.o dll2_hash.o -o th
exit 0
#endif

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
