
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

/** \file PageCache_internal.H
 * \brief Internal implementation declarations for PageCache
 * \author Phillip F Knaack */

#include "dll2.H"
#include <stdlib.h>
#include <stdio.h>

/** a DLL2 link index for PageCachePage */
enum PAGE_CACHE_LIST_INDICES {
    PAGE_LIST, PAGE_HASH, PAGE_LRU_LOCK, PAGE_DIRTY, NUM_PAGE_LISTS };

/** Internal representation of a PageCachePage
 *
 * This object is derived from PageCachePage and adds linked list
 * pointers and a reference count. */
class PCPInt : public PageCachePage {
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
    /** this object can be on 3 linked lists simultaneously,
     * \see PAGE_CACHE_LIST_INDICES */
    LListLinks<PCPInt>  links[NUM_PAGE_LISTS];
    /** Constructor.
     * \param _page_number The page number of this page. */
    PCPInt(int _page_number) : PageCachePage(_page_number) { refcount = 0; }
    /** helper which checks reference count. */
    bool is_locked(void) { return (refcount != 0); }
};

/** DLL2 hash helper class for PageCachePage */
class PCPIntHashCompare {
public:
    static int hash_key( PCPInt * item ) {
        return item->get_page_number();
    }
    static int hash_key( int key ) {
        return key;
    }
    static bool hash_key_compare( PCPInt * item, int key ) {
        return (item->get_page_number() == key);
    }
};

/** a DLL2 container class for PageCachePage objects.
 *
 * This object contains: <ul>
 *   <li> a linked list of all PageCachePage objects
 *   <li> a hash of PageCachePage objects where hash key is page number
 *   <li> a least-recently used list of all unlocked PageCachePage objects
 *   <li> a linked list of all locked PageCachePage objects.
 * </ul> */
class PageCachePageList {
    /** A linked list of all objects.
     * \see PAGE_CACHE_LIST_INDICES */
    LList     <PCPInt,                      PAGE_LIST>  list;
    /** A hash table of objects, hash key is page number.
     * \see PAGE_CACHE_LIST_INDICES */
    LListHash <PCPInt,int,PCPIntHashCompare,PAGE_HASH>  hash;
    /** A least-recently-used list of all unlocked objects,
     * \see PAGE_CACHE_LIST_INDICES */
    LListLRU  <PCPInt,                  PAGE_LRU_LOCK>  lru;
    /** A linked list of all locked objects,
     * \see PAGE_CACHE_LIST_INDICES */
    LList     <PCPInt,                  PAGE_LRU_LOCK>  locklist;
    LList     <PCPInt,                     PAGE_DIRTY>  dirty;
public:
    /** @name Statistics */
    // @{
    /** return the count of pages currently cached.
     * \return the count of pages currently cached. */
    int get_cnt    (void) { return list.get_cnt(); }
    /** return the count of pages currently unlocked.
     * \return the count of pages currently unlocked. */
    int get_lru_cnt(void) { return lru .get_cnt(); }
    // @}
    /** @name Search/Retrieval */
    // @{
    /** locate a page in cache by its page number.
     * \param page_number the number of the page to search for in the hash
     * \return a pointer to the page or NULL if not found. */
    PCPInt * find( int page_number ) { return hash.find( page_number ); }
    /** return the head of the linked list of all objects.
     * \return head of the linked list of all objects
     * \note The PageCachePageList object should not be manipulated
     *  while the linked list is being walked.  Otherwise the results
     *  may be undefined. */
    PCPInt * get_head  (void      ) { return list.get_head ( ); }
    /** return the next object in the linked list.
     * \param i the current item in the linked list
     * \return the next object in the linked list 
     * \note The PageCachePageList object should not be manipulated
     *  while the linked list is being walked.  Otherwise the results
     *  may be undefined. */
    PCPInt * get_next  (PCPInt * i) { return list.get_next (i); }
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
        list.add(p);
        hash.add(p);
        if (locked) {
            locklist.add(p);
            if (p->ref() != 1) {
                fprintf(stderr, "PageCachePageList::add: inconsistent lock!\n");
                exit( 1 );
            }
        } else {
            lru.add(p);
        }
        if (p->is_dirty() && !dirty.onthislist(p))
            dirty.add(p);
    }
    /** remove a PageCachePage from the container.
     * \param p the PageCachePage to remove */
    void remove( PCPInt * p ) {
        list.remove(p);
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
            locklist.add(p);
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
            lru.add(p);
        }
        if (p->is_dirty() && !dirty.onthislist(p))
            dirty.add(p);
        if (!p->is_dirty() && dirty.onthislist(p))
            dirty.remove(p);
    }
};
