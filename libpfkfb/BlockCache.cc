/*
    This file is part of the "pfkutils" tools written by Phil Knaack
    (pfk@pfk.org).
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

/** \file BlockCache.cc
 * \brief The BlockCache implementation
 * \author Phillip F Knaack
 * 
 * This file implements all the methods of the BlockCache object.
 * This object manages arbitrary accesses to blocks of a file, using
 * underlying PageIO and PageCache objects. */

/** \page BlockCache BlockCache object

The PageCache object is consumed by the BlockCache object.  This
object abstracts the concept of arbitrarily-sized blocks in a file.
When the BlockCache::get() method is called, it determines how many
pages are required to satisfy the request.  If the entire request is
within a single page, the returned object is optimized by returning a
pointer directly into the PageCachePage object.  Since pages are
generally pretty large, most BlockCache operations are thus within a
single page.  Thus, this optimization results in dramatic zero-copy
speedup.  If the request spans more than one page, then a separate
memory buffer is allocated, and the block object maintains references
to all pages.

Various optimizations are also done when the \c for_write parameter is
specified.  Obviously if the user is requesting only a portion of a
page, then for_write cannot be passed to PageCache, because this could
trash the remainder of the page which is not being modified.  However
in multi-block fetches, for_write does have an effect if complete
pages are involved in the block.

When the BlockCache::flush() method is invoked, it will eventually
call PageCache::flush().  This will automatically synchronize all
single-page blocks with the data file; however for multi-page blocks,
some additional work must be done before flushing the PageCache.  All
portions of the multi-page blocks are copied to their respective pages
prior to calling PageCache::flush().

Be sure to call BlockCacheBlock::mark_dirty() to ensure that flushes are
written to the containing pages properly.

\note While it is legal for PageCache users to call PageCache::get()
on the same page twice (because of internal reference counts, etc),
this is \b NOT legal for BlockCache blocks.  Undefined behavior and data
corruption will occur if the same or overlapping blocks are requested
from BlockCache at the same time.

Next: \ref FileBlock

*/

#include "BlockCache.h"
#include "BlockCache_internal.h"

#include <string.h>

BlockCache :: BlockCache( PageIO * _io, int max_bytes )
{
    io = _io;
    pc = new PageCache( io, max_bytes / PageIO::PCP_PAGE_SIZE );
    bcl = new BlockCacheList;
}

BlockCache :: ~BlockCache( void )
{
    delete bcl;
    delete pc;
    delete io;
}

BlockCacheBlock *
BlockCache :: get( off_t offset, int size, bool for_write )
{
    off_t ending_offset;
    int starting_page, ending_page, num_pages, pg, i;
    int offset_in_starting_page;
    int offset_in_ending_page;
    BCB * ret;

    ending_offset = offset + size - 1;

    starting_page = offset / PageIO::PCP_PAGE_SIZE;
    ending_page = ending_offset / PageIO::PCP_PAGE_SIZE;
    num_pages = ending_page - starting_page + 1;

    offset_in_starting_page = offset % PageIO::PCP_PAGE_SIZE;
    offset_in_ending_page = ending_offset % PageIO::PCP_PAGE_SIZE;

    ret = new BCB(offset,size);
    bcl->list.add_tail(ret);

    ret->num_pages = num_pages;
    ret->pages = new PageCachePage*[num_pages];

    uint8_t * uptr = NULL;
    int remaining = 0;
    int pg_offset = offset_in_starting_page;

    if (num_pages > 1)
    {
        // need a temporary buf
        ret->ptr = new uint8_t[size];

        if (!for_write)
        {
            // copy out of cache pages.
            uptr = ret->ptr;
            remaining = size;
        }
    }

    for (pg=starting_page, i=0; i < num_pages; pg++,i++)
    {
        bool for_write_pg = false;
        if (for_write && size >= PageIO::PCP_PAGE_SIZE)
        {
            if (pg != starting_page && pg != ending_page)
            {
                // if its a multi-page block and we're in a middle
                // block, do get_for_write
                for_write_pg = true;
            }
            else if (pg == starting_page && offset_in_starting_page == 0)
            {
                // if the block takes up the entire first page, 
                // get that page for_write.
                for_write_pg = true;
            }
            else if (pg == ending_page &&
                     offset_in_ending_page == (PageIO::PCP_PAGE_SIZE-1))
            {
                // if the block takes up the entire ending page,
                // get that page for_write.
                for_write_pg = true;
            }
            // else must read+modify+write.
        }
        PageCachePage * p = pc->get(pg, for_write_pg);
        ret->pages[i] = p;
        if (uptr && remaining > 0)
        {
            int to_copy = remaining;
            if (to_copy > (PageIO::PCP_PAGE_SIZE - pg_offset))
                to_copy = PageIO::PCP_PAGE_SIZE - pg_offset;
            memcpy(uptr, p->get_ptr() + pg_offset, to_copy);
            remaining -= to_copy;
            uptr += to_copy;
            pg_offset = 0; // following page starts at beginning
        }
    }

    if (num_pages == 1)
        // allow ptr to point directly into the page cache
        ret->ptr = ret->pages[0]->get_ptr() + offset_in_starting_page;

    if (for_write)
    {
        memset(ret->ptr, 0, size);
        ret->dirty = true;
    }

    return ret;
}

void
BlockCache :: release( BlockCacheBlock * _bcb, bool dirty )
{
    BCB * bcb = (BCB *)_bcb;
    int i;

    if (bcb->dirty)
        dirty = true;
    bcl->list.remove(bcb);

    uint8_t * uptr = NULL;
    int remaining = 0;
    int pg_offset = 0;

    if (dirty && bcb->num_pages > 1)
    {
        uptr = bcb->ptr;
        remaining = bcb->size;
        pg_offset = bcb->offset % PageIO::PCP_PAGE_SIZE;
    }

    for (i=0; i < bcb->num_pages; i++)
    {
        PageCachePage * p = bcb->pages[i];
        if (uptr && remaining > 0)
        {
            int to_copy = remaining;
            if (to_copy > (PageIO::PCP_PAGE_SIZE - pg_offset))
                to_copy = PageIO::PCP_PAGE_SIZE - pg_offset;
            memcpy(p->get_ptr() + pg_offset, uptr, to_copy);
            remaining -= to_copy;
            uptr += to_copy;
            pg_offset = 0;
        }
        pc->release(p, dirty);
    }

    if (bcb->num_pages > 1)
        delete[] bcb->ptr;

    delete bcb;
}

void
BlockCache :: truncate( off_t offset )
{
    BCB * bcb;

    for (bcb = bcl->list.get_head();
         bcb;
         bcb = bcl->list.get_next(bcb))
    {
        if (bcb->get_offset() >= offset)
        {
            fprintf(stderr, "ERROR: BlockCache :: truncate: "
                    "block at offset %lld is still in use!\n",
                    (long long) bcb->get_offset());
            return;
        }
    }

    int pages = (offset + PageIO::PCP_PAGE_SIZE - 1) / PageIO::PCP_PAGE_SIZE;

    pc->truncate_pages(pages);
}

void
BlockCache :: flush_bcb(BlockCacheBlock * _bcb)
{
    BCB * bcb = (BCB *)_bcb;
    if (bcb->num_pages == 1 || bcb->dirty == false)
        // if its a single-page object, then the ptr points
        // directly into a page, so there's nothing to sync up.
        return;
    uint8_t * uptr = bcb->ptr;
    int remaining = bcb->size;
    int pg_offset = bcb->offset % PageIO::PCP_PAGE_SIZE;
    int i;

    for (i=0; i < bcb->num_pages; i++)
    {
        PageCachePage * p = bcb->pages[i];
        if (remaining > 0)
        {
            int to_copy = remaining;
            if (to_copy > (PageIO::PCP_PAGE_SIZE - pg_offset))
                to_copy = PageIO::PCP_PAGE_SIZE - pg_offset;
            memcpy(p->get_ptr() + pg_offset, uptr, to_copy);
            p->mark_dirty();
            remaining -= to_copy;
            uptr += to_copy;
            pg_offset = 0;
        }
    }

    // the pages are now synced with the bcb.
    bcb->dirty = false;
}

void
BlockCache :: flush( void )
{
    BCB * bcb;

    // first copyback all multi-page bcb's to their respective
    // page objects; then sync up the page cache.

    for (bcb = bcl->list.get_head();
         bcb;
         bcb = bcl->list.get_next(bcb))
    {
        flush_bcb(bcb);
    }

    pc->flush();
}
