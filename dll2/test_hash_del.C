
#include "test_hash.H"

void
del_entries( thing_hash * h, thing_list * l )
{
    thing * t;
    while ( t = l->dequeue_head() )
    {
        h->remove( t );
        delete t;
    }
}
