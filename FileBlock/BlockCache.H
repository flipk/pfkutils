
#ifndef __BLOCK_CACHE_H__
#define __BLOCK_CACHE_H__

#include "PageCache.H"

class BlockCacheList;

class BlockCacheBlock {
    friend class BlockCache;
protected:
    off_t offset;
    int size;
    UCHAR * ptr;
    int num_pages;
    bool dirty;
    PageCachePage ** pages;
    BlockCacheBlock( off_t _offset, int _size ) {
        offset = _offset; size = _size; dirty = false;
        // ptr, num_pages, and pages are NOT initialized here.
        pages = NULL;
    }
    ~BlockCacheBlock(void) {
        /* BlockCache users can't delete this obj */
        if (pages)
            delete[] pages;
    }
public:
    off_t   get_offset(void) { return offset; }
    int     get_size  (void) { return size;   }
    UCHAR * get_ptr   (void) { return ptr;    }
    void    mark_dirty(void) { dirty = true;  }
};

class BlockCache {
    PageIO    * io;
    PageCache * pc;
    BlockCacheList * bcl;
    void flush_bcb(BlockCacheBlock * bcb);
public:
    BlockCache( PageIO * _io, int max_bytes );
    // note the destructor does NOT delete io!
    ~BlockCache( void );
    BlockCacheBlock * get( off_t offset, int size, bool for_write );
    void release( BlockCacheBlock * bcb, bool dirty );
    void flush(void);
};

#endif /* __BLOCK_CACHE_H__ */
