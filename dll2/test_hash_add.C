
#include <stdlib.h>

#include "test_hash.H"

void
add_entries( thing_hash * h, thing_list * l )
{
    thing * t;
    int i;

    for ( i = 0; i < 1000000; i++ )
    {
        t = new thing;
        t->a = random();
        l->add( t );
        h->add( t );
    }
}
