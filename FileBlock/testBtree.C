
#include "Btree.H"

#include <string.h>
#include <stdio.h>

struct empkey {
    UINT32_t id;
    //
    UCHAR * ptr(void) { return (UCHAR*) this; }
    int size(void) { return sizeof(*this); }
    static int max_size(void) { return sizeof(empkey); }
};

struct employee {
    UINT32_t id;
    char name[128];
    //
    UCHAR * ptr(void) { return (UCHAR*) this; }
    int size(void) { return sizeof(*this)-sizeof(name)+strlen(name)+1; }
    static int max_size(void) { return sizeof(employee); }
};

int
main()
{
    Btree * bt = Btree::open(NULL);

    BTDatum <empkey>   key (bt);
    BTDatum <employee> data(bt);

    key.alloc();
    key.d->id.set( 4 );

    bt->get( &key, &data );
}
