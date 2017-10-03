
/** \file BlockCache.C
 * \brief The BlockCache implementation
 * \author Phillip F Knaack
 * 
 * This file implements all the methods of the BlockCache object.
 * This object manages arbitrary accesses to blocks of a file, using
 * underlying PageIO and PageCache objects. */

#include "BlockCache.H"
#include "BlockCache_internal.H"

#include <string.h>

BlockCache :: BlockCache( PageIO * _io, int max_bytes )
{
    io = _io;
    pc = new PageCache( io, max_bytes / PageCache::PAGE_SIZE );
    bcl = new BlockCacheList;
}

BlockCache :: ~BlockCache( void )
{
    delete bcl;
    delete pc;
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

    starting_page = offset / PageCache::PAGE_SIZE;
    ending_page = ending_offset / PageCache::PAGE_SIZE;
    num_pages = ending_page - starting_page + 1;

    offset_in_starting_page = offset % PageCache::PAGE_SIZE;
    offset_in_ending_page = ending_offset % PageCache::PAGE_SIZE;

    ret = new BCB(offset,size);
    bcl->list.add(ret);

    ret->num_pages = num_pages;
    ret->pages = new PageCachePage*[num_pages];

    UCHAR * uptr = NULL;
    int remaining = 0;
    int pg_offset = offset_in_starting_page;

    if (num_pages > 1)
    {
        // need a temporary buf
        ret->ptr = new UCHAR[size];

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
        if (for_write && size >= PageCache::PAGE_SIZE)
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
                     offset_in_ending_page == (PageCache::PAGE_SIZE-1))
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
            if (to_copy > (PageCache::PAGE_SIZE - pg_offset))
                to_copy = PageCache::PAGE_SIZE - pg_offset;
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

    UCHAR * uptr = NULL;
    int remaining = 0;
    int pg_offset = 0;

    if (dirty && bcb->num_pages > 1)
    {
        uptr = bcb->ptr;
        remaining = bcb->size;
        pg_offset = bcb->offset % PageCache::PAGE_SIZE;
    }

    for (i=0; i < bcb->num_pages; i++)
    {
        PageCachePage * p = bcb->pages[i];
        if (uptr && remaining > 0)
        {
            int to_copy = remaining;
            if (to_copy > (PageCache::PAGE_SIZE - pg_offset))
                to_copy = PageCache::PAGE_SIZE - pg_offset;
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
BlockCache :: flush_bcb(BlockCacheBlock * _bcb)
{
    BCB * bcb = (BCB *)_bcb;
    if (bcb->num_pages == 1 || bcb->dirty == false)
        // if its a single-page object, then the ptr points
        // directly into a page, so there's nothing to sync up.
        return;
    UCHAR * uptr = bcb->ptr;
    int remaining = bcb->size;
    int pg_offset = bcb->offset % PageCache::PAGE_SIZE;
    int i;

    for (i=0; i < bcb->num_pages; i++)
    {
        PageCachePage * p = bcb->pages[i];
        if (remaining > 0)
        {
            int to_copy = remaining;
            if (to_copy > (PageCache::PAGE_SIZE - pg_offset))
                to_copy = PageCache::PAGE_SIZE - pg_offset;
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
