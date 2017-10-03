
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

/** \file FileBlockLocalAun.C
 * \brief Everything need to manage AUs: allocate, free, and retrieval.
 * \author Phillip F Knaack
 */

#include "FileBlockLocal.H"

#include <stdlib.h>

FB_AUN_T
FileBlockLocal :: alloc_aun( AUHead * au, int size )
{
    // be sure to account for size of _AUHead 
    size += _AUHead::used_size;

    int num_aus = SIZE_TO_AUS(size);

    // take first au off this bucket list -> au

    dequeue_bucket(num_aus, au);

    FB_AUN_T aun = au->get_aun();

    fh.mark_dirty();

    // toggle used-flag to true
    au->d->used(true);
    au->mark_dirty();

    int free_area_size = au->d->size();

    // if size is not exact match
    if (free_area_size != num_aus)
    {
    //   create new free region at end of this region
        AUHead  new_free(bc);

        FB_AUN_T new_free_aun = aun + num_aus;            
        int new_free_size = 0;
        FB_AUN_T next_next_aun = 0;

        if (free_area_size != 0)
        {
            new_free_size = free_area_size - num_aus;
            next_next_aun = aun + free_area_size;
            fh.d->info.free_aus.decr( num_aus );
        }
        fh.d->info.used_aus.incr( num_aus );

        if (!new_free.get(new_free_aun, true))
        {
            fprintf(stderr, "unable to fetch aun %d\n", new_free_aun);
            exit(1);
        }
        new_free.d->used(false);
        new_free.mark_dirty();

        if (next_next_aun != 0)
        {
    //   get next-next region and set prev field to next region
            AUHead  next_next(bc);
            next_next.get(next_next_aun);
            next_next.d->prev.set( new_free_aun );
            next_next.mark_dirty();
        }

    //   set next region's prev and size fields
        new_free.d->size( new_free_size );
        new_free.d->prev.set( aun );

    //   modify this region's size field
        au->d->size( num_aus );

        enqueue_bucket( &new_free );

        if (new_free_size == 0)
        {
            // if we just moved the end marker, update num_aus
            fh.d->info.num_aus.set(new_free_aun);
        }
    }
    else
    {
        fh.d->info.free_extents.decr();
        fh.d->info.free_aus.decr( num_aus );
        fh.d->info.used_aus.incr( num_aus );
    }
    fh.d->info.used_extents.incr();

    return aun;
}

void
FileBlockLocal :: free_aun( FB_AUN_T aun )
{
    // retrieve au
    AUHead au(bc);
    if (!au.get(aun))
    {
        fprintf(stderr, "FileBlockLocal :: free : unable to get %d\n", aun);
        return;
    }
    au.mark_dirty();
    fh.mark_dirty();

    FB_AUN_T prev_aun, next_aun=0, next_next_aun=0;
    FB_AUN_T au_size, prev_size = 0, next_size = 0;
    bool prev_free = false, next_free = false;

    au_size = au.d->size();
    prev_aun = au.d->prev.get();
    if (au_size != 0)
        next_aun = aun + au_size;

    // retrieve predecessor and successor au's
    AUHead prev_au(bc), next_au(bc), next_next_au(bc);
    if (prev_aun != 0)
    {
        if (!prev_au.get(prev_aun))
        {
            fprintf(stderr, "ERROR: unable to get prev %d\n", prev_aun);
            return;
        }
        if (prev_au.d->used() == false)
        {
            prev_free = true;
            prev_size = prev_au.d->size();
            remove_bucket( &prev_au );
            prev_au.mark_dirty();
        }
    }
    if (next_aun != 0)
    {
        if (!next_au.get(next_aun))
        {
            fprintf(stderr, "ERROR: unable to get next %d\n", next_aun);
            return;
        }
        if (next_au.d->used() == false)
        {
            next_free = true;
            next_size = next_au.d->size();
            remove_bucket( &next_au );
            if (next_size != 0)
            {
                next_next_aun = next_aun + next_size;
                next_next_au.get(next_next_aun);
                next_next_au.mark_dirty();
            }
            next_au.mark_dirty();
        }
    }
    else
    {
        fprintf(stderr, "ERROR: wtf???\n");
        /*DEBUGME*/
        exit(1);
    }

    // coalesce free space with predecessor and successor;
    // the following cases exist:
    //  1 both prev and next free
    //      use prev, delete current and next, and coalesce
    //  2 prev free only
    //      use prev, delete current, and coalesce
    //  3 next free only
    //      use current, delete next, and coalesce
    //  4 neither free
    //      mark current as free, no coalescing

    fh.d->info.used_aus.decr( au_size );
    fh.d->info.used_extents.decr();

    if (prev_free)
    {
        if (next_free)
        {
            /**  \test case 1 coalescing, not end of file.
             * Allocate three regions, then free the first, third,
             * and finally the second, and verify that all three are
             * coalesced into one region.  also validate all statistics.
             * ensure the three regions
             * are not at the end of the file. */
            /** \test case 1 coalescing, end of file.
             * Repeat previous test, but ensure the third region
             * is the end-of-file marker. 
             * also validate all statistics. */
            // case 1
            if (next_size == 0)
            {
                prev_au.d->size( 0 );
                fh.d->info.num_aus.set( prev_aun );
                fh.d->info.free_aus.decr( prev_size + next_size );
            }
            else
            {
                prev_au.d->size( prev_size + au_size + next_size );
                next_next_au.d->prev.set( prev_aun );
                fh.d->info.free_aus.incr( au_size );
            }
            enqueue_bucket( &prev_au );
            fh.d->info.free_extents.decr();
        }
        else
        {
            /** \test case 2 coalescing.
             * Allocate three regions, then free the first, then 
             * free the second.  verify it coalesces the second with
             * the first. 
             * also validate all statistics. */
            // case 2
            prev_au.d->size( prev_size + au_size );
            next_au.d->prev.set( prev_aun );
            enqueue_bucket( &prev_au );
            fh.d->info.free_aus.incr( au_size );
        }
    }
    else if (next_free)
    {
        /** \test case 3 coalescing.
         * Allocate three regions, then free the second, then the first.
         * verify it coalesces the first with the second.
         * also validate all statistics */
        // case 3
        au.d->used(false);
        if (next_size == 0)
        {
            au.d->size( 0 );
            fh.d->info.num_aus.set( aun );
        }
        else
        {
            au.d->size( au_size + next_size );
            next_next_au.d->prev.set( aun );
            fh.d->info.free_aus.incr( au_size );
        }
        enqueue_bucket( &au );
    }
    else
    {
        // case 4
        au.d->used(false);
        enqueue_bucket( &au );
        fh.d->info.free_aus.incr( au_size );
        fh.d->info.free_extents.incr();
    }
}

FileBlockInt *
FileBlockLocal :: get_aun( FB_AUN_T aun, bool for_write )
{
    AUHead au(bc);

    if (!au.get(aun))
        return NULL;

    if (au.d->used() == false)
    {
        fprintf(stderr, "FileBlockLocal :: get_aun: au %d is free!\n", aun);
        /*DEBUGME*/
        exit(1);
    }

    FileBlockInt * fb = new FileBlockInt;

    off_t pos  = aun          * AU_SIZE;
    int   size = au.d->size() * AU_SIZE;

    au.release();

    pos  += _AUHead::used_size;
    size -= _AUHead::used_size;

    BlockCacheBlock * bcb = bc->get( pos, size, for_write );
    if (!bcb)
    {
        delete fb;
        return NULL;
    }

    fb->set_bcb( bcb );
    active_blocks.add(fb);

    return fb;
}
