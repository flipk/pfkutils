/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

#include "test_hash.H"

void
del_entries( thing_hash * h, thing_list * l )
{
    thing * t;
    while ( (t = l->dequeue_head()) != NULL )
    {
        h->remove( t );
        delete t;
    }
}
