/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

/** \file ExtentMap.C
 * \brief Implementation of Extents and Extent objects
 * \author Phillip F Knaack */

#include "ExtentMap.H"

#include <stdlib.h>
#include <string.h>

/** Allocation of patch of Extent objects.
 *
 * For memory efficiency, Extent objects are not allocated and
 * freed one at a time; they are allocated in pages and maintained
 * on a free list.  Since Extent objects are themselves rather small,
 * this dramatically improves memory allocator overhead. */
class ExtentsPage {
public:
    /** A linked list of all ExtentsPage objects for deletion 
        when Extents is deleted. */
    LListLinks <ExtentsPage> links[1];
    /** A number chosen out of a bithat for how many Extent objects
        live in a single object. */
    static const int EXTENTS_PER_PAGE = 4096;
    /** the actual group of Extent objects */
    Extent extents[ EXTENTS_PER_PAGE ];
};

/** Memory allocator for Extent objects.
 *
 * This class manages free Extent objects.  They're small so the
 * malloc/free overhead would be terrible.  Make a list of ExtentsPages
 * and a list of free Extent objects, and deliver them upon request. */
class ExtentFreePool {
    /** a list of all free Extent objects. */
    ExtentOrderedList    list;
    /** a list of all ExtentsPage objects. */
    LList <ExtentsPage,0> pages;
public:
    /** destructor; empties list and deletes all ExtentsPage objs */
    ~ExtentFreePool(void) {
        Extent * e;
        ExtentsPage * p;
        while ((e = list.dequeue_head()) != NULL)
            ;
        while ((p = pages.dequeue_head()) != NULL)
            delete p;
    }
    /** allocate a new Extent.
     *
     * This method pulls a free Extent off of the list; or if the list
     * is empty, allocates a new ExtentsPage. */
    Extent * alloc(off_t _offset, UINT32 _size, UINT32 _id=0) { 
        if (list.get_cnt() == 0)
        {
            ExtentsPage * p = new ExtentsPage;
            pages.add(p);
            for (int i = 0; i < ExtentsPage::EXTENTS_PER_PAGE; i++)
                list.add( &p->extents[i] );
        }
        Extent * e = list.dequeue_head();
        e->offset = _offset;
        e->size = _size;
        e->id = _id;
        if (_id != 0)
            e->used = 1;
        else
            e->used = 0;
        return e;
    }
    /** free an Extent back to the free list */
    void free(Extent * e) {
        list.add(e);
    }
};

/** here is the pool of all free Extent objects */
static ExtentFreePool extent_pool;

Extents :: Extents( void )
{
    memset(bucket_bitmap, 0, sizeof(bucket_bitmap));
    count_used = 0;
    count_free = 0;
}

Extents :: ~Extents( void )
{
    Extent * e, * ne;
    for (e = list.get_head(); e; e = ne)
    {
        ne = list.get_next(e);
        list.remove(e);
        if (e->used)
        {
            idhash.remove(e);
        }
        else
            remove_from_bucket(e);
        extent_pool.free(e);
    }
}

void
Extents :: print( void )
{
    Extent * e;
    int i = 0;
    for ( e = get_head(); e; e = get_next(e) )
    {
        printf( "%d : offset 0x%llx size 0x%x (%s)",
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
        e = idhash.find(id);
    } while (e != NULL);

    return id;
}

void
Extents :: add( off_t _offset, UINT32 _size )
{
    Extent * e = extent_pool.alloc( _offset, _size );
    list.add(e);
    add_to_bucket(e);
    count_free ++;
}

void
Extents :: add( off_t _offset, UINT32 _size, UINT32 _id )
{
    Extent * e = extent_pool.alloc( _offset, _size, _id );
    list.add(e);
    idhash.add(e);
    count_used ++;
}

Extent *
Extents :: find_id( UINT32 id )
{
    return idhash.find(id);
}

/** \note Sizes are always rounded up to the nearest 32-byte boundary.
 *  The bucket-list search algorithm depends on this (it uses a 5-bit
 *  shift to index the bucket-list).  Without this guarantee, the allocation
 *  algorithm may return overlapping allocations.
 */
Extent *
Extents :: _alloc( UINT32 id, UINT32 size )
{
    // round up to next 32-byte boundary.
    size = ((((size - 1) >> 5) + 1) << 5);

    if (id != 0)
        // free the old one before we look for a new one
        free_id( id );

    int b;
    Extent * e = NULL;

    for ( b = size_to_bucket(size);
          b < NUM_BUCKETS;
          b = find_next_bucket(b+1) )
    {
        e = buckets[b].get_first();
        if (e)
            break;
    }

    if (b == NUM_BUCKETS)
    {
        fprintf(stderr,"ERROR! NO BUCKETS??\n");
        exit(1);
    }

    buckets[b].remove(e);

    if (buckets[b].get_cnt() == 0)
        clear_bit(b);

    // allocate a new id if we don't already have one.
    if (id == 0)
        id = alloc_id();

    // regardless of what happens next, we have
    // created a new 'used' extent.  if we are
    // converting a perfect-match 'free', we must
    // also decrement count_free.  however if
    // we are splitting a free one, count_free
    // actually remains unchanged.
    count_used ++;

    if (e->size == size)
    {
        // flip this one to used and return it.
        e->used = 1;
        e->id = id;
        idhash.add(e);
        count_free --;
        return e;
    }

    // split this one in two.
    Extent * ne = extent_pool.alloc( e->offset, size, id );
    // the last entry on the list represents 
    // the 'remainder' of the file (basically out to
    // infinity) so its size never really decreases.
    if (list.get_tail() != e)
        e->size -= size;
    e->offset += size;
    list.add_before(ne, e);
    add_to_bucket(e);
    idhash.add(ne);

    return ne;
}

void
Extents :: free_id( UINT32 id )
{
    Extent * e = find_id(id);
    if (e)
        free(e);
}

void
Extents :: free( Extent * e )
{
    if (!e->used)
        return;

    // remove from hash before clearing id!
    idhash.remove(e);
    e->id = 0;
    e->used = 0;
    count_used --;

    Extent * e2;

    e2 = list.get_prev(e);
    if (e2 && e2->used == 0)
    {
        e->offset = e2->offset;

        if (((UINT64)e->size + (UINT64)e2->size) > 0x7FFFFFFFULL)
        {
            // This can happen if we are freeing a lot of things
            // in a file and end up with a 2GB hole in the middle
            // of the file.  In this case, don't combine the entries,
            // just make two free-entries in a row.
            goto dont_combine_1;
        }
        e->size += e2->size;
        list.remove(e2);
        remove_from_bucket(e2);
        count_free --;
        extent_pool.free(e2);
    }
dont_combine_1:
    e2 = list.get_next(e);
    if (e2 && e2->used == 0)
    {
        // if we are freeing the last entry before the end of
        // the list, then we don't need to update the size of the
        // end of the list, because the last entry on the list always
        // has a hardcoded size of 2^31-1.
        if (list.get_tail() != e2)
        {
            if (((UINT64)e->size + (UINT64)e2->size) > 0x7FFFFFFFULL)
            {
                // This can happen if we are freeing a lot of things
                // in a file and end up with a 2GB hole in the middle
                // of the file.  In this case, don't combine the entries,
                // just make two free-entries in a row.
                goto dont_combine_2;
            }
            e->size += e2->size;
        }
        else
            e->size = 0x7FFFFFFF;
        list.remove(e2);
        remove_from_bucket(e2);
        count_free --;
        extent_pool.free(e2);
    }
dont_combine_2:
    // add to bucket after updating size!
    add_to_bucket(e);
    count_free ++;
}
