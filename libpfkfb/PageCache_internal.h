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

/** \file PageCache_internal.h
 * \brief Internal implementation declarations for PageCache
 */

#include "dll3.h"
#include <stdlib.h>
#include <stdio.h>


class PCPInt;
class PCPIntHashCompare;
typedef DLL3::List< PCPInt, 1, false > PageList_t;
typedef DLL3::Hash< PCPInt, int, PCPIntHashCompare, 2, false > PageHash_t;
typedef DLL3::List< PCPInt, 3, false > PageLruLock_t;
typedef DLL3::List< PCPInt, 4, false > PageDirtyList_t;

/** Internal representation of a PageCachePage
 *
 * This object is derived from PageCachePage and adds linked list
 * pointers and a reference count. */
class PCPInt : public PageCachePage,
               public PageList_t::Links,
               public PageHash_t::Links,
               public PageLruLock_t::Links,
               public PageDirtyList_t::Links
{
    /** Reference count; 0 means unlocked, >0 means locked */
    int refcount;
    /** only PageCachePageList can change reference counts on
     * this object. */
    friend class PageCachePageList;
    /** increment reference count */
    int ref(void) { return ++refcount; }
    /** decrement reference count.
     * \note Decrementing below zero is a fatal error. */
    int deref(void) {
        if (refcount == 0)
        {
            fprintf(stderr, "error, dereferencing a page already at count=0\n");
            exit(1);
        }
        return --refcount;
    }
public:
    /** Constructor.
     * \param _page_number The page number of this page. */
    PCPInt(uint64_t _page_number)
        : PageCachePage(_page_number) { refcount = 0; }
    /** helper which checks reference count. */
    bool is_locked(void) { return (refcount != 0); }
};

class PCPIntHashCompare {
public:
    static uint32_t obj2hash(const PCPInt &item)
    { return (uint32_t) item.get_page_number(); }
    static uint32_t key2hash(const uint64_t key)
    { return (uint32_t) key; }
    static bool hashMatch(const PCPInt &item, const uint64_t key)
    { return (item.get_page_number() == key); }
};

/** a DLL3 container class for PageCachePage objects.
 *
 * This object contains: <ul>
 *   <li> a linked list of all PageCachePage objects
 *   <li> a hash of PageCachePage objects where hash key is page number
 *   <li> a least-recently used list of all unlocked PageCachePage objects
 *   <li> a linked list of all locked PageCachePage objects.
 * </ul> */
class PageCachePageList {
    PageList_t       pageList;
    PageHash_t       hash;
    PageLruLock_t    lru;
    PageLruLock_t    locklist;
    PageDirtyList_t  dirty;
public:
    /** @name Statistics */
    // @{
    /** return the count of pages currently cached.
     * \return the count of pages currently cached. */
    int get_cnt    (void) { return pageList.get_cnt(); }
    /** return the count of pages currently unlocked.
     * \return the count of pages currently unlocked. */
    int get_lru_cnt(void) { return lru .get_cnt(); }
    // @}
    /** @name Search/Retrieval */
    // @{
    /** locate a page in cache by its page number.
     * \param page_number the number of the page to search for in the hash
     * \return a pointer to the page or NULL if not found. */
    PCPInt * find( uint64_t page_number ) { return hash.find( page_number ); }
    /** return the head of the linked list of all objects.
     * \return head of the linked list of all objects
     * \note The PageCachePageList object should not be manipulated
     *  while the linked list is being walked.  Otherwise the results
     *  may be undefined. */
    PCPInt * get_head  (void      ) { return pageList.get_head ( ); }
    /** return the next object in the linked list.
     * \param i the current item in the linked list
     * \return the next object in the linked list 
     * \note The PageCachePageList object should not be manipulated
     *  while the linked list is being walked.  Otherwise the results
     *  may be undefined. */
    PCPInt * get_next  (PCPInt * i) { return pageList.get_next (i); }
    /** return the oldest item in the least-recently-used list.
     * \return the oldest item in the least-recently-used list. */
    PCPInt * get_oldest(void      ) { return lru.get_oldest( ); }


    int      get_dirty_cnt(void) { return dirty.get_cnt(); }
    PCPInt * get_dirty_head(void) { return dirty.get_head(); }
    PCPInt * get_dirty_next(PCPInt * i) { return dirty.get_next(i); }

    // @}
    /** @name Manipulation */
    // @{
    /** add a PageCachePage to the container.
     * \param p the PageCachePage to add
     * \param locked indicate whether the item is to be locked or not;
     *    if locked, the item will be put on the locklist; else it will
     *    be put on the LRU */
    void add( PCPInt * p, bool locked ) { 
        pageList.add_tail(p);
        hash.add(p);
        if (locked) {
            locklist.add_tail(p);
            if (p->ref() != 1) {
                fprintf(stderr, "PageCachePageList::add: inconsistent lock!\n");
                exit( 1 );
            }
        } else {
            lru.add_head(p);
        }
        if (p->is_dirty() && !dirty.onthislist(p))
            dirty.add_tail(p);
    }
    /** remove a PageCachePage from the container.
     * \param p the PageCachePage to remove */
    void remove( PCPInt * p ) {
        pageList.remove(p);
        hash.remove(p);
        if (p->is_locked())
            locklist.remove(p);
        else
            lru.remove(p);
        if (dirty.onthislist(p))
            dirty.remove(p);
    }
    /** mark a PageCachePage as referenced.
     * \param p the PageCachePage to reference
     *
     * If the page was not previously referenced, it will be locked
     * and therefore moved from the LRU to the locklist. */
    void ref( PCPInt * p ) {
        if (p->ref() == 1) {
            lru.remove(p);
            locklist.add_tail(p);
        }
    }
    /** dereference a PageCachePage.
     * \param p the PageCachePage to dereference
     * 
     * If the page's reference count drops to zero as a result of this
     * operation, the object will be unlocked and therefore moved from
     * the locklist to the LRU. */
    void deref( PCPInt * p ) {
        if (p->deref() == 0) {
            locklist.remove(p);
            lru.add_head(p);
        }
        if (p->is_dirty() && !dirty.onthislist(p))
            dirty.add_tail(p);
        if (!p->is_dirty() && dirty.onthislist(p))
            dirty.remove(p);
    }
};
