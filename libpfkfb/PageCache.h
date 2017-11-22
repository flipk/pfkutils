/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

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

/** \file PageCache.h
 * \brief Define interfaces: PageCache, PageCachePage
 */

#ifndef __PAGE_CACHE_H__
#define __PAGE_CACHE_H__

#include "PageIO.h"

#include <sys/types.h>
#include <inttypes.h>
#include <unistd.h>

class PageCachePage;
class PageCachePageList;

/** A cache of PageCachePage objects fetched thru a PageIO object.
 *
 * This object will fetch pages from a file using the PageIO interface
 * and return pointers to the user.  It will also maintain reference counts
 * on pages so that multiple callers may reference the same page multiple
 * times.  To prevent thrashing on the filesystem, pages are only written
 * back thru PageIO when the caching limit has been reached or when a flush
 * operation is requested. */
class PageCache {
    /** PageCache must limit number of pages it will cache */
    int max_pages;
    /** the PageIO interface for getting data in/out of a file */
    PageIO * io;
    /** the compound data type for storing the cached pages. */
    PageCachePageList * pgs;
    bool printFlushCount;
public:
    /** Constructor.
     * \param _io the PageIO that will be used to fetch and put pages.
     * \param _max_pages the number of pages that we will be allowed to
     *  keep in memory; this limits the memory utilization of this object
     *  to resonable levels. */
    PageCache( PageIO * _io, int _max_pages );
    /** Destructor.
     * \note This destructor deletes the PageIO object too!
     * \note All in-use pages should be released prio to destroying this
     *   object! */
    ~PageCache(void);
    /** return the internal PageIO object.
     * \note This function bypasses the cache and can thus be dangerous!
     *   Improper use may corrupt the file contents due to incoherent cache.
     * \return A pointer to the PageIO in use by this object.
     *
     * Assumption is that user may want to store other information in
     * this file; this interface allows the user to access file not managed
     * by the cache. */
    PageIO * get_io(void) { return io; }
    /** Retrieve a page from the file.
     * \param page_number the page number of the page to be retrieved.
     * \param for_write If the user intends to write the entire page and does
     *  not care to read anything from the file, this parameter allows an 
     *  optimization where an empty page is returned without invoking PageIO
     *  to read it.
     * \return A pointer to a PageCachePage object for the page.
     *
     * If the page is currently in the cache, this will return a pointer
     * to the cached page and increase the page's reference count.  If the
     * page is not in cache, the PageIO interface is accessed to retrieve
     * the page from the file. */
    PageCachePage * get(uint64_t page_number, bool for_write);
    /** Release a page from user's access.
     * \param p A PageCachePage previously returned by the get() method.
     * \param dirty The user must indicate if he modified this page, so that
     *  we can decide if we have to access PageIO to write it back again.
     *
     * This method dereferences the page and puts it back on an LRU. If 
     * the user modified the page, the page must be marked as dirty, so that
     * it will eventually be written back to the file (thru the PageIO
     * interface. */
    void release( PageCachePage * p, bool dirty );
    /** truncate a file.
     */
    void truncate_pages(uint64_t num_pages);
    /** Flush the cache, force synchronization.
     *
     * This method walks the list of all dirty pages, as well as all 
     * locked pages, and writes all changes thru the PageIO interface. */
    void flush(void);
};

/** Access object for a page of a file.
 *
 * This object contains one page of a file. The user may access the
 * contents of the page through a pointer in this object.  The user
 * must notify this object (thru the mark_dirty method) if he has
 * modified the contents of this data. */
class PageCachePage {
    friend class PageCache;
    /** the number of the page in the file this object contains */
    uint64_t page_number;
    /** indicates whether the data has been modified */
    bool dirty;
protected:
    /** allocate memory for the page and initialize member fields.
     * \param _page_number the number of the page being referenced.
     * \note this constructor does not populate the contents of the page;
     *   it is assumed the caller will do that. */
    PageCachePage(uint64_t _page_number) {
        dirty = false;  page_number = _page_number;
        ptr = new uint8_t[PageIO::PCP_PAGE_SIZE];
    }
    /** destructor frees memory for the page.
     * \note this destructor does not write the contents back to the file.
     *   it is assumed the caller will do that. */
    ~PageCachePage(void) { delete[] ptr; }
    /** a pointer to the page data itself. */
    uint8_t * ptr;
public:
    /** access method to return the page number. */
    uint64_t get_page_number(void) const { return page_number; }
    /** access method to get the data pointer. */
    uint8_t * get_ptr(void) const { return ptr; }
    /** user must call this if he has modified the page data. */
    void mark_dirty(void) { dirty = true; }
    bool is_dirty(void) const { return dirty; }
};

#endif /* __PAGE_CACHE_H__ */
