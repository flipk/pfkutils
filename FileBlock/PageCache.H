
#ifndef __PAGE_CACHE_H__
#define __PAGE_CACHE_H__

#include "types.H"

#include <sys/types.h>
#include <unistd.h>

class PageIO;
class PageCachePage;
class PageCachePageList;

class PageCache {
    int max_pages;
    PageIO * io;
    PageCachePageList * pgs;
public:
    static const int PAGE_SIZE = 65536;
    PageCache( PageIO * _io, int _max_pages );
    ~PageCache(void);
    PageIO * get_io(void) { return io; }
    PageCachePage * get(int page_number, bool for_write);
    void release( PageCachePage * p, bool dirty );
    void flush(void);
};

class PageCachePage {
    friend class PageCache;
    int page_number;
    bool dirty;
protected:
    PageCachePage(int _page_number) {
        dirty = false;  page_number = _page_number;
        ptr = new UCHAR[PageCache::PAGE_SIZE];
    }
    ~PageCachePage(void) { delete[] ptr; }
    UCHAR * ptr;
public:
    int get_page_number(void) { return page_number; }
    UCHAR * get_ptr(void) { return ptr; }
    void mark_dirty(void) { dirty = true; }
};

class PageIO {
public:
    virtual ~PageIO(void) { /* placeholder */ }
    // return false if some error occurred
    virtual bool  get_page( PageCachePage * pg ) = 0;
    virtual bool  put_page( PageCachePage * pg ) = 0;
    virtual int   get_num_pages(bool * page_aligned = NULL) = 0;
    virtual off_t get_size(void) = 0;
};

// following is an example of how to create a PageIO object.
// this one is useful for local files.  just open(2) the file
// and pass the fd to this class.

class PageIOFileDescriptor : public PageIO {
    int fd;
public:
    PageIOFileDescriptor(int _fd);
    // note destructor does NOT close fd!
    ~PageIOFileDescriptor(void);
    // return false if some error occurred
    bool get_page( PageCachePage * pg );
    bool put_page( PageCachePage * pg );
    int get_num_pages(bool * page_aligned = NULL);
    off_t get_size(void);
};

#endif /* __PAGE_CACHE_H__ */
