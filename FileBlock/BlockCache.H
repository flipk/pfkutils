
/** \file BlockCache.H
 * \brief the BlockCache interface definition
 * \author Phillip F Knaack
 *
 * This file defines the interface to a BlockCache. */

#ifndef __BLOCK_CACHE_H__
#define __BLOCK_CACHE_H__

#include "PageCache.H"

class BlockCacheList;

/** \brief what the user sees when interfacing to the cache */
class BlockCacheBlock {
    friend class BlockCache;
protected:
    off_t offset;
    int size;
    /** a pointer to a buffer the user can modify; if this block
     * fits in a page, this pointer points directly to the page 
     * in the PageCache; however if it crosses a page boundary,
     * this is a pointer to a private buffer which is copied out
     * of the PageCache and then copied back when modified. */
    UCHAR * ptr;
    /** the number of PageCache pages which this block references.
     * if greator than 1, indicates the block crosses at least one
     * page boundary. */
    int num_pages;
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
    off_t   get_offset(void) { return offset; }
    int     get_size  (void) { return size;   }
    UCHAR * get_ptr   (void) { return ptr;    }
    void    mark_dirty(void) { dirty = true;  }
};

/** \brief Interface to get and put arbitrary-sized blocks
 * in a file.
 * 
 * This object manages blocks of arbitrary size.  They can be fetched,
 * held for as long as needed, and put when done; multiple blocks can
 * be held at a time.  If the user does not modify the block, an 
 * optimization is possible where the block is not written back to the
 * file-- the memory is simply freed.  The user indicates this with the
 * \b dirty flag. */

class BlockCache {
    /** \brief this is how the object actually accesses the file. */
    PageIO    * io;
    /** \brief a cache on top of the io interface. */
    PageCache * pc;
    /** \brief a list of all outstanding blocks in use by the caller. */
    BlockCacheList * bcl;
    /** \brief internal helper for flushing a block back to the file. */
    void flush_bcb(BlockCacheBlock * bcb);
public:
    /** \brief constructor.
     * \param _io a PageIO object which is used for all accesses to the file.
     * \param max_bytes the maximum number of bytes that should reside in
     *   the cache. */
    BlockCache( PageIO * _io, int max_bytes );
    /** \brief destructor.
     * \note This destructor does \b NOT delete the io object! it is assumed
     * the user may want to use io for other accesses for other purposes. */
    ~BlockCache( void );
    /** \brief get a block from the file.
     * \param offset the position in the file where the block starts.
     * \param size the size of the block in the file.
     * \param for_write if the user intends to write the entire block and has
     *   no intention of reading any of it, an optimization may be possible.
     * \return a BlockCacheBlock for the user to read or write. */
    BlockCacheBlock * get( off_t offset, int size, bool for_write = false );
    /** \brief release a block
     * \param bcb the BlockCacheBlock previously returned by get().
     * \param dirty the user should indicate if the block was modified. */
    void release( BlockCacheBlock * bcb, bool dirty );
    /** \brief flush all open blocks back to the physical file. */
    void flush(void);
};

#endif /* __BLOCK_CACHE_H__ */
