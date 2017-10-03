
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

/** \file FileBlockLocalBucket.C
 * \brief Everything necessary to managing free-AU bucket lists.
 * \author Phillip F Knaack
 */


/** \page AUNBuckets  FileBlock AUN Bucket Lists

Every free region is on a bucket list.  There are 2048 bucket lists,
each one representing regions of a particular size, from 1 to 2047 AUs
in size.  Each bucket list is a doubly-linked list.

There is also a bucket bitmap.  Each bucket list head pointer has a
corresponding bit in this bitmap.  If the list is empty, the bit is zero.
If the list is nonempty, the bit is one.

When an allocation of a particular size is requested, that specific
bucket is checked first for a region of exactly the right size.  If
that bucket list is empty, the search proceeds up to the next bucket
list for regions of the next size.  The bucket bitmap is used to
accelerate this search.  The final bucket list (2048) is reserved for
an end-of-file marker, whose size is considered infinite.

When a matching free region is found (even if it is the infinite-sized
end-of-file marker), this free region is split into two regions: a
used region for the allocation, and another region containing the
remainder of the original free region.  (For the end-of-file marker,
the new region continues to be marked as infinite.)

When a region is freed, the next and previous regions are checked to
see if they are also free.  If they are, the current region is
combined with them to make one contiguous free region.

The resulting free region is then inserted into the head of a bucket
list, corresponding to the size of the region.

Next: \ref AUIDMGMT

*/


#include "FileBlockLocal.H"

#include <stdlib.h>

int
FileBlockLocal :: ffu_bucket(int num_aus)
{
    BucketBitmap * bm = &fh.d->bucket_bitmap;
    int bucket = aus_to_bucket(num_aus);

    // race thru bucket bitmap finding first nonempty bucket list
    while (bucket < BucketList::NUM_BUCKETS)
    {
        if ((bucket & 0x1F) == 0)
            if (bm->longs[bucket/32].get() == 0)
            {
                /* skip a whole long */
                bucket += 32;
                continue;
            }
        if (bm->get_bit(bucket) == 1)
            break;
        bucket++;
    }

    if (bucket == BucketList::NUM_BUCKETS)
    {
        fprintf(stderr, "FileBlockLocal :: alloc:   WTF ??????\n");
        /*DEBUGME*/
        exit(1);
    }

    return bucket;
}

bool
FileBlockLocal :: dequeue_bucketn( int bucket,  AUHead * au )
{
    BucketList   * bl = &fh.d->bucket_list;
    BucketBitmap * bm = &fh.d->bucket_bitmap;
    FB_AUN_T aun;

    aun = bl->list_head[bucket].get();
    if (aun == 0)
        return false;

    if (!au->get(aun))
    {
        fprintf(stderr, "FileBlockLocal :: alloc : WTF? DEBUGME\n");
        /*DEBUGME*/
        exit(1);
    }

    // set bucket head to free_area->bucket_next
    FB_AUN_T bucket_next = au->d->bucket_next();
    bl->list_head[bucket].set( bucket_next );

    au->d->bucket_next(0);
    au->d->bucket_prev(0);
    au->mark_dirty();

    if (bucket_next != 0)
    {
        // follow bucket_next, and set its bucket_prev to 0.
        AUHead bucket_next_au(bc);
        bucket_next_au.get(bucket_next);
        bucket_next_au.d->bucket_prev(0);
        bucket_next_au.mark_dirty();
    }
    else
    {
        // if bucket list empty, clear bit in bitmap
        bm->set_bit(bucket,false);
    }

    return true;    
}

void
FileBlockLocal :: enqueue_bucket(AUHead *au)
{
    BucketList   * bl = &fh.d->bucket_list;
    BucketBitmap * bm = &fh.d->bucket_bitmap;
    int bucket = aus_to_bucket(au->d->size());
    FB_AUN_T head_aun;

    head_aun = bl->list_head[bucket].get();
    au->d->bucket_next( head_aun );
    au->d->bucket_prev( 0 );
    bl->list_head[bucket].set( au->get_aun() );
    au->mark_dirty();

    if (head_aun != 0)
    {
        AUHead  head_au(bc);
        head_au.get(head_aun);
        head_au.d->bucket_prev( au->get_aun() );
        head_au.mark_dirty();
    }
    else
    {
        //   if bucket list went empty->nonempty, set bit in bitmap
        bm->set_bit(bucket,true);
    }
}

void
FileBlockLocal :: remove_bucket(AUHead *au)
{
    BucketList   * bl = &fh.d->bucket_list;
    BucketBitmap * bm = &fh.d->bucket_bitmap;

    FB_AUN_T bucket_prev = au->d->bucket_prev();
    FB_AUN_T bucket_next = au->d->bucket_next();

    // if bucket_prev is zero
    //   update bl head to be my bucket_next
    //   if bl head is now 0
    //     clear bit.
    // else
    //   fetch bucket_prev
    //   update its bucket_next to be my bucket_next

    if (bucket_prev == 0)
    {
        int bucket = aus_to_bucket(au->d->size());
        fh.mark_dirty();
        bl->list_head[bucket].set( bucket_next );
        if (bucket_next == 0)
            bm->set_bit(bucket,false);
    }
    else
    {
        AUHead  bpau(bc);
        bpau.get(bucket_prev);
        bpau.d->bucket_next( bucket_next );
        bpau.mark_dirty();
    }

    au->d->bucket_next(0);
    au->d->bucket_prev(0);
    au->mark_dirty();

    // if bucket_next is not zero
    //   fetch bucket_next
    //   update its bucket_prev to be my bucket_prev

    if (bucket_next != 0)
    {
        AUHead  bnau(bc);
        bnau.get(bucket_next);
        bnau.d->bucket_prev( bucket_prev );
        bnau.mark_dirty();
    }
}
