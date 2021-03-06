/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/

/** \file FileBlockLocalAun.cc
 * \brief Everything need to manage AUs: allocate, free, and retrieval.
 */

/** \page AUNMGMT  FileBlock AUN Management

The first level of management in the file is lists of regions.  Every 
region is on a global ordered list.  When a region is freed, this allows
fast lookups of previous and successive regions, to determine if coalescing
of free space is possible.

Every region stores the following information:

<ul>
<li> The AUN of the previous region.  If this is the first region in the
     file, this value is zero.
<li> The size of the region, in AUs.  This field is 31-bits, therefore
     the largest region which can be expressed in this field is 2^31*32=64GB.
     The size value of 0 is reserved for the last region in the file, also
     known as an end-of-file marker.
<li> There is one bit for a used/free indicator.
<li> A used region contains:
   <ul>
   <li> The AUID value used to represent this item.  The value of 0 is
        reserved to indicate an AUID table or AUID free-stack L2/L3 page.
   </ul>
<li> A free region contains:
   <ul>
   <li> A bucket_prev pointer for the bucket linked list.
   <li> A bucket_next pointer for the bucket linked list.
   </ul>
</ul>

Note that an AUN of 0 (zero) indicates an invalid AUN or an end-of-list
marker.

Next: \ref AUNBuckets

*/


//redhat needs this for PRIu64 and friends.
#define __STDC_FORMAT_MACROS 1

#include "FileBlockLocal.h"

#include <stdlib.h>

// a fun bug was discovered here.  everything over NUM_BUCKETS-2 aus in size
// get grouped in a single bucket, NUM_BUCKETS-1.  but there is a case where
// alloc_aun doesn't check that the bucket just dequeued was actually big
// enough for the allocation, if "size" requested legitimately goes in
// that bucket.  if it picks an AU that is smaller than the size requested,
// the memcpy of data into that allocation will overwrite the next
// AUHead, corrupting the file.
// the solution i chose was to put a max on the largest allocation you
// can ask for and enforce that in FileBlockLocal::alloc. also because the
// app in question wanted 64k and i like 64k as a number, i doubled
// NUM_BUCKETS so that 64k works.

FB_AUN_T
FileBlockLocal :: alloc_aun( FB_AUN_T desired_aun, AUHead * au, int size )
{
    // be sure to account for size of _AUHead 
    size += _AUHead::used_size;

    int num_aus = SIZE_TO_AUS(size);
    FB_AUN_T aun;

    // take first au off this bucket list -> au

    if (desired_aun == 0)
    {
        dequeue_bucket(num_aus, au);
        aun = au->get_aun();
    }
    else
    {
        aun = desired_aun;
        if (!au->get(aun))
        {
            fprintf(stderr, "WTF?? FileBlockLocal :: alloc_aun: crap\n");
            /*DEBUGME*/
            exit(1);
        }
        if (au->d->used())
        {
            fprintf(stderr, "FileBlockLocal :: alloc_aun: used???\n");
            /*DEBUGME*/
            exit(1);
        }
        remove_bucket(au);
    }

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
            fprintf(stderr, "unable to fetch aun %"
                    PRIu64 "\n", new_free_aun);
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
        fprintf(stderr, "FileBlockLocal :: free : unable to get %"
                PRIu64 "\n", aun);
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
            fprintf(stderr, "ERROR: unable to get prev %"
                    PRIu64 "\n", prev_aun);
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
            fprintf(stderr, "ERROR: unable to get next %"
                    PRIu64 "\n", next_aun);
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
        }
        next_au.mark_dirty();
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
        fprintf(stderr, "FileBlockLocal :: get_aun: au %"
                PRIu64 " is free!\n", aun);
        /*DEBUGME*/
        exit(1);
    }

    FileBlockInt * fb = new FileBlockInt;

    off_t pos  = (off_t)aun   * AU_SIZE;
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
    active_blocks.add_tail(fb);

    return fb;
}
