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

/** \file BlockCache.h
 * \brief the BlockCache interface definition
 *
 * This file defines the interface to a BlockCache. */

#ifndef __BLOCK_CACHE_H__
#define __BLOCK_CACHE_H__

#include <sys/types.h>
#include <inttypes.h>
#include "PageCache.h"

class BlockCacheList;

/** what the user sees when interfacing to the cache */
class BlockCacheBlock {
    friend class BlockCache;
protected:
    /** the position in the file of this block */
    off_t offset;
    /** the size of this block on disk */
    int size;
    /** a pointer to a buffer the user can modify. if this block
     * fits in a page, this pointer points directly to the page 
     * in the PageCache; however if it crosses a page boundary,
     * this is a pointer to a private buffer which is copied out
     * of the PageCache and then copied back when modified. */
    uint8_t * ptr;
    /** the number of PageCache pages which this block references.
     * if greator than 1, indicates the block crosses at least one
     * page boundary. */
    uint64_t num_pages;
    /** indicates if the block has been modified by the user. */
    bool dirty;
    /** pointer to array of PageCachePage objects.  the size of this
     * array is indicated by num_pages. */
    PageCachePage ** pages;
    /** constructor that populates the offset and size.
     * \note ptr, num_pages, and pages are \b NOT initialized here. */
    BlockCacheBlock( off_t _offset, int _size ) {
        offset = _offset; size = _size; dirty = false; pages = NULL;
    }
    /** BlockCache users can't delete this object.  The correct way to
     * delete this object is by calling the release method. */
    ~BlockCacheBlock(void) {
        if (pages)
            delete[] pages;
    }
public:
    off_t     get_offset(void) { return offset; }
    int       get_size  (void) { return size;   }
    uint8_t * get_ptr   (void) { return ptr;    }
    void      mark_dirty(void) { dirty = true;  }
};

/** Interface to get and put arbitrary-sized blocks in a file.
 * 
 * This object manages blocks of arbitrary size.  They can be fetched,
 * held for as long as needed, and put when done; multiple blocks can
 * be held at a time.  If the user does not modify the block, an 
 * optimization is possible where the block is not written back to the
 * file-- the memory is simply freed.  The user indicates this with the
 * \b dirty flag. */

class BlockCache {
    /** this is how the object actually accesses the file. */
    PageIO    * io;
    /** a cache on top of the io interface. */
    PageCache * pc;
    /** a list of all outstanding blocks in use by the caller. */
    BlockCacheList * bcl;
    /** internal helper for flushing a block back to the file. */
    void flush_bcb(BlockCacheBlock * bcb);
public:
    /** constructor.
     * \param _io a PageIO object which is used for all accesses to the file.
     * \param max_bytes the maximum number of bytes that should reside in
     *   the cache. */
    BlockCache( PageIO * _io, int max_bytes );
    /** destructor.
     * \note This destructor deletes the PageIO object! */
    ~BlockCache( void );
    /** get a block from the file.
     * \param offset the position in the file where the block starts.
     * \param size the size of the block in the file.
     * \param for_write if the user intends to write the entire block and has
     *   no intention of reading any of it, an optimization may be possible.
     * \return a BlockCacheBlock for the user to read or write. */
    BlockCacheBlock * get( off_t offset, int size, bool for_write = false );
    /** release a block.
     * \param bcb the BlockCacheBlock previously returned by get().
     * \param dirty the user should indicate if the block was modified.
     * \note if bcb->mark_dirty() has been called, this is equivalent to
     *  passing dirty=true to this function (that is, if bcb->mark_dirty()
     *  has been called, the dirty argument to this function is ignored,
     *  treated as if it was true. */
    void release( BlockCacheBlock * bcb, bool dirty = false );
    /** truncate the file to a given position. */
    void truncate( off_t offset );
    /** \brief flush all open blocks back to the physical file. */
    void flush(void);
};

/** provide a brief way to access a disk block with a type struct.
 * Define any type T using platform-independent data types, such as
 * from types.h.  Then invoke this template and call get(offset), and
 * then reference the data pointer.  Don't worry about releasing, since
 * another get(offset) or deleting this object will cause an automatic
 * release.  You can call the release method manually to force a release,
 * in case you have an ordering requirement. \note Always release or destroy
 * the object first if you plan on freeing the space in the file. Also
 * be sure to call mark_dirty if you have modified the data!
 * \param T the data type to be encapsulated by this template.
 */
template <class T>
struct BlockCacheT
{
    /** this object needs to remember the BlockCache object. */
    BlockCache * bc;
    /** a pointer to the block that was retrieved */
    BlockCacheBlock * bcb;
    /** constructor takes a BlockCache pointer
     * \param _bc the BlockCache object to get and put T from */
    BlockCacheT(BlockCache * _bc) { bc = _bc; bcb = NULL; d = NULL; }
    /** destructor automatically releases the block back to the file */
    ~BlockCacheT(void) { release(); }
    /** fetch from the file; automatically determines size of T.
     * \param pos the position in the file where T resides
     * \param for_write set to true if you don't care what T's previous
     *        value in the file was and you're overwriting it from scratch
     * \return true if the block was fetched OK and false if not. */
    bool get( off_t pos, bool for_write = false ) {
        release();
        bcb = bc->get( pos, sizeof(T), for_write );
        if (!bcb) return false;
        d = (T *) bcb->get_ptr();
        return true;
    }
    /** release the block back to the file */
    void release(void) {
        if (bcb) bc->release(bcb);
        bcb = NULL;
        d = NULL;
    }
    /** mark the block as dirty if you modified it */
    void mark_dirty(void) { if (bcb) bcb->mark_dirty(); }
    /** a pointer to the actual data */
    T * d;
    off_t get_offset(void) { return bcb ? bcb->get_offset() : 0; }
};

#endif /* __BLOCK_CACHE_H__ */
