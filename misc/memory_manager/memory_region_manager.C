
/*
    This file is part of the "pfkutils" tools written by Phil Knaack
    (pknaack1@netscape.net).
    Copyright (C) 2008  Phillip F Knaack

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#define ULONG unsigned int
#define NULL 0

#include "memory_region_manager.H"

memory_region_manager :: memory_region_manager( ULONG size, ULONG hashsize )
    : used_pointer_hash( hashsize )
{
    memory_region * r = alloc_memory_region();
    int bm;

    peak_bytes_used = bytes_used = used_regions = 0;
    pool_size = size;
    memory_base = (ULONG) ::malloc( size );
    if ( memory_base == 0 )
    {
        //xxx swfm about suckage
    }
    r->init( memory_base, size );
    regions.add( r );
    bm = size_to_bucket(size);
    free_buckets[ bm ].add( r );
}

memory_region_manager :: ~memory_region_manager( void )
{
    memory_region * r, * nr;
    for ( r = regions.get_head(); r; r = nr )
    {
        nr = regions.get_next(r);
        if ( r->is_used() )
            used_pointer_hash.remove( r );
        else
            free_buckets[ size_to_bucket( r->size_get() ) ].remove( r );
        regions.remove( r );
    }
    for ( r = region_free_list.get_head(); r; r = nr )
    {
        nr = region_free_list.get_next(r);
        region_free_list.remove( r );
    }
    memory_region_page * rp, * nrp;
    for ( rp = region_page_list.get_head(); rp; rp = nrp )
    {
        nrp = region_page_list.get_next(rp);
        region_page_list.remove( rp );
        delete rp;
    }
    ::free( (void*) memory_base );
}

void *
memory_region_manager :: malloc( ULONG size )
{
    memory_region * r, * r2;
    ULONG  bucket, bucket2, ret = 0;
    Region_bucket_list * rbl;

    // round up to nearest BUCKET_MIN multiple.
    size = (size+(BUCKET_MIN-1))&(~(BUCKET_MIN-1));

    for ( bucket = size_to_bucket( size ); bucket < NUM_BUCKETS; bucket++ )
    {
        rbl = &free_buckets[bucket];
        for ( r = rbl->get_head(); r; r = rbl->get_next(r) )
        {
            if ( r->size_get() >= size )
                break;
        }
        if ( r )
            break;
    }

    if ( r == NULL )
    {
        // xxx swfm, out of memory
        printf( "out of memory!\n" );
        return (void*) 0;
    }

    if ( r->size_get() == size )
    {
        // exact size match, move from free list to used list
        mark_region_used( r );
        rbl->remove( r );
        used_pointer_hash.add( r );
        ret = r->start_get();
    }
    else
    {
        // split this region into two pieces
        r2 = alloc_memory_region();
        r2->init( r->start_get(), size );

        ULONG newsize = r->size_get() - size;
        bucket2 = size_to_bucket( newsize );
        r->init( r->start_get() + size, newsize );

        regions.add_before( r2, r );
        used_pointer_hash.add( r2 );
        mark_region_used( r2 );

        if ( bucket2 != bucket )
        {
            rbl->remove( r );
            free_buckets[bucket2].add( r );
        }

        ret = r2->start_get();
    }
    return (void*) ret;
}

void
memory_region_manager :: free( void * _pointer )
{
    ULONG pointer = (ULONG) _pointer;
    memory_region * r, * r2, * r3;
    int bb, ba, bm;

    r = used_pointer_hash.find( memory_region::hash_key( pointer ));
    if ( r == NULL )
    {
        printf( "not found!\n" );
        return;
    }

    used_pointer_hash.remove( r );
    mark_region_free( r );

    // look for neighbors on the region list 
    // for candidates for coalescing

    r2 = regions.get_prev( r );
    if ( r2 && r2->is_used() )
        // remove it from consideration
        r2 = NULL;
    r3 = regions.get_next( r );
    if ( r3 && r3->is_used() )
        // remove it from consideration
        r3 = NULL;

    // case 1, coalesce with both
    if ( r2 && r3 )
    {
        bb = size_to_bucket( r2->size_get() );
        r2->end_set( r3->end_get() );
        ba = size_to_bucket( r2->size_get() );

        regions.remove( r );
        region_free_list.add( r );

        regions.remove( r3 );
        region_free_list.add( r3 );

        bm = size_to_bucket(r3->size_get());
        free_buckets[bm].remove(r3);

        if ( bb != ba )
        {
            free_buckets[bb].remove( r2 );
            free_buckets[ba].add( r2 );
        }
    }

    // case 2, coalesce with prev
    else if ( r2 )
    {
        bb = size_to_bucket( r2->size_get() );
        r2->end_set( r->end_get() );
        ba = size_to_bucket( r2->size_get() );

        regions.remove( r );
        region_free_list.add( r );

        if ( bb != ba )
        {
            free_buckets[bb].remove( r2 );
            free_buckets[ba].add( r2 );
        }
    }

    // case 3, coalesce with next
    else if ( r3 )
    {
        bb = size_to_bucket( r3->size_get() );
        r3->start_set( r->start_get() );
        ba = size_to_bucket( r3->size_get() );

        regions.remove( r );
        region_free_list.add( r );

        if ( bb != ba )
        {
            free_buckets[bb].remove( r3 );
            free_buckets[ba].add( r3 );
        }
    }

    // case 4, coalesce with nothing
    else
    {
        bm = size_to_bucket( r->size_get());
        free_buckets[ bm ].add( r );
    }
}
