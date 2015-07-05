
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

/** \file PageCache.cc
 * \brief Implements PageCache and PageIOFileDescriptor.
 * \author Phillip F Knaack */

/** \page PageCache PageCache object

The PageIO interface is consumed by a PageCache object.  This is a
cache of PageCachePage objects up to a maximum number.  The method
PageCache::get() will retrieve a page from the cache if it is in
cache, or fetch it from the PageIO interface if it is not (and add it
to the cache).  A page returned by PageCache::get() is in the \c locked
state with a \c reference \c count greator than 0.  When the caller is
done using the page object, it must call PageCache::release() to
return the page to the cache.  This decreases the reference count on
the page.  When the PageCache::flush() method is called, all pages
currently in cache are synced up with the backend storage, by invoking
the PageIO interface to write any pages marked as dirty.

If the number of pages cached in PageCache reaches the maximum, then a
true \c Least-Recently-Used (LRU) algorithm is used to determine which
old page to remove from cache.  If the oldest page is clean (in sync
with the PageIO backend storage) then the memory is simply freed.  If
the oldest page is dirty, then PageIO::put_page() is invoked prior to
freeing the page memory.

When the \c for_write flag is set on a PageCache::get() operation, this
indicates that the user has no intention of using any data currently
stored in the page, and instead intends to write data to the whole
page.  This results in an optimization where the PageIO::get_page()
method is not called, instead an empty PageCachePage is built and
returned to the user.  (Why read a page from the file if you're just
going to throw the data away and write the whole page with new data
anyway?)

\note The maximum count only applies to unlocked pages.  A locked
page does not count against the limit.

Be sure to call PageCachePage::mark_dirty() to ensure that flushes are
written to the file properly.

Next: \ref BlockCache 

*/

#include "PageCache.h"
#include "PageCache_internal.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

PageCache :: PageCache( PageIO * _io, int _max_pages )
{
    io = _io;
    max_pages = _max_pages;
    pgs = new PageCachePageList;
    time( &last_flush );
}

PageCache :: ~PageCache(void)
{
    PCPInt * p;
    flush();
    while ((p = pgs->get_head()) != NULL)
    {
        if (p->is_locked())
        {
            fprintf(stderr, "error page is locked at delete time!\n");
            exit(1);
        }
        pgs->remove(p);
        delete p;
    }
    delete pgs;
}

PageCachePage *
PageCache :: get(int page_number, bool for_write)
{
    PCPInt * ret;
    ret = pgs->find( page_number );
    if (ret != NULL)
    {
        pgs->ref(ret);
        if (for_write)
        {
            memset(ret->ptr, 0, PC_PAGE_SIZE);
            ret->dirty = true;
        }
        return ret;
    }
    ret = new PCPInt( page_number );
    if (for_write)
    {
        memset(ret->ptr, 0, PC_PAGE_SIZE);
        ret->dirty = true;
    }
    else
    {
        if (!io->get_page(ret))
        {
            fprintf(stderr, "error getting page %d\n", page_number);
            exit( 1 );
        }
    }
    pgs->add(ret,true);
    return ret;
}

void
PageCache :: release( PageCachePage * _p, bool dirty )
{
    PCPInt * p = (PCPInt *)_p;
    if (dirty)
        p->dirty = true;
    pgs->deref(p);
    while (pgs->get_lru_cnt() > max_pages)
    {
        p = pgs->get_oldest();
        pgs->remove(p);
        if (p->dirty)
        {
            if (!io->put_page(p))
            {
                fprintf(stderr, "error putting page %d\n",
                        p->page_number);
                exit( 1 );
            }
        }
        delete p;
    }
    time_t now;
    if ( time( &now ) != last_flush )
    {
        last_flush = now;
        flush();
    }
}

void
PageCache :: truncate_pages(int num_pages)
{
    PCPInt * p, * np;
    for (p = pgs->get_head(); p; p = np)
    {
        np = pgs->get_next(p);
        if (p->page_number >= num_pages)
        {
            if (p->is_locked())
            {
                fprintf(stderr, "ERROR: PageCache :: truncate_pages: "
                        "page %d is still locked\n", p->page_number);
                return;
            }
            else
                pgs->remove(p);
        }
    }
    io->truncate_pages(num_pages);
}

/** compare page numbers of two PCPInt objects, for qsort.
 * \param _a the first PCPInt to compare
 * \param _b the second PCPInt to compare
 * \return positive if _a's page number is greator than _b's page number,
 *         negative if _a's page number is less than _b's page number,
 *         or zero if they are equal.
 * \relates PageCache
 *
 * This function compares two PCPInt objects.  Its purpose is to be a
 * utility function to the standard C library function qsort, to assist
 * qsort in sorting an array of PCPInt objects.  */

static int
page_compare( const void * _a, const void * _b )
{
    PCPInt * a = *(PCPInt **)_a;
    PCPInt * b = *(PCPInt **)_b;
    if (a->get_page_number() > b->get_page_number())
        return 1;
    if (a->get_page_number() < b->get_page_number())
        return -1;
    return 0;
}

void
PageCache :: flush(void)
{
    PCPInt * p;
    int i, count;

    count = pgs->get_dirty_cnt();
    if (count == 0)
        return;

    PCPInt * pages[count];
    i = 0;
    for (p = pgs->get_dirty_head(); p; p = pgs->get_dirty_next(p))
        pages[i++] = p;

    qsort( pages, count, sizeof(PCPInt*),
           (int(*)(const void *, const void *))page_compare );

    printf("flushing %d pages\n", count);

    for (i = 0; i < count; i++)
    {
        p = pages[i];
        pgs->ref(p);
        if (!io->put_page(p))
        {
            fprintf(stderr, "error putting page %d\n", p->page_number);
            exit( 1 );
        }
        // the page is now synced with the file.
        p->dirty = false;
        pgs->deref(p);
    }
}
