
#include "ExtentMap.H"

#include <stdlib.h>
#include <string.h>

Extents :: Extents( void )
{
    memset(bucket_bitmap, 0, sizeof(bucket_bitmap));
}

Extents :: ~Extents( void )
{
    Extent * e, * ne;
    for (e = list.get_head(); e; e = ne)
    {
        ne = list.get_next(e);
        list.remove(e);
        if (e->used)
            hash.remove(e);
        else
            remove_from_bucket(e);
        delete e;
    }
}

void
Extents :: print( void )
{
    Extent * e;
    int i = 0;
    for ( e = list.get_head();
          e;
          e = list.get_next(e) )
    {
        printf( "%d : offset %lld size %d (%s)",
                i++, e->offset, e->size, e->used ? "USED" : "FREE");
        if (e->used)
            printf(" id %#x", e->id);
        printf("\n");
    }
}

UINT32
Extents :: alloc_id( void )
{
    UINT32 id;
    Extent * e;

    do {
        do {
            id = random();
        } while (id == 0 || id == 0xFFFFFFFFU);
        e = hash.find(id);
    } while (e != NULL);

    return id;
}

void
Extents :: add( off_t _offset, UINT32 _size )
{
    Extent * e = new Extent( _offset, _size );
    list.add(e);
    add_to_bucket(e);
}

void
Extents :: add( off_t _offset, UINT32 _size, UINT32 _id )
{
    Extent * e = new Extent( _offset, _size, _id );
    list.add(e);
    hash.add(e);
}

// locate a block by its ID number.
Extent *
Extents :: find( UINT32 id )
{
    return hash.find(id);
}

Extent *
Extents :: alloc( UINT32 size )
{
    // round up 'size' to the nearest 32-byte boundary.
    size = ((((size - 1) >> 5) + 1) << 5);

    int b;
    UINT32 id;
    Extent * e;

    for ( b = size_to_bucket(size);
          b < NUM_BUCKETS;
          b = find_next_bucket(b+1) )
    {
        e = buckets[b].dequeue_head();
        if (e)
            break;
    }

    if (b == NUM_BUCKETS)
    {
        printf("ERROR! NO BUCKETS??\n");
        exit(1);
    }

    if (buckets[b].get_cnt() == 0)
        clear_bit(b);

    id = alloc_id();

    if (e->size == size)
    {
        // flip this one to used and return it.
        e->used = 1;
        e->id = id;
        hash.add(e);
        return e;
    }

    // split this one in two.
    Extent * ne = new Extent( e->offset, size, id );
    e->size -= size;
    e->offset += size;
    list.add_before(ne, e);
    add_to_bucket(e);
    hash.add(ne);

    return ne;
}

void
Extents :: free( UINT32 id )
{
    Extent * e = find(id);
    if (e)
        free(e);
}

void
Extents :: free( Extent * e )
{
    if (!e->used)
        return;

    // remove from hash before clearing id!
    hash.remove(e);
    e->id = 0;
    e->used = 0;

    Extent * e2;

    e2 = list.get_prev(e);
    if (e2 && e2->used == 0)
    {
        e->offset = e2->offset;
        e->size += e2->size;
        list.remove(e2);
        remove_from_bucket(e2);
        delete e2;
    }
    e2 = list.get_next(e);
    if (e2 && e2->used == 0)
    {
        e->size += e2->size;
        list.remove(e2);
        remove_from_bucket(e2);
        delete e2;
    }

    // add to bucket after updating size!
    add_to_bucket(e);
}
