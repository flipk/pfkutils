
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
//    static const int PAGE_SIZE = 65536;
    static const int PAGE_SIZE = 1024; // xxx testing
    PageCache( PageIO * _io, int _max_pages );
    ~PageCache(void);
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
};

class PageIO {
public:
    virtual ~PageIO(void) { /* placeholder */ }
    // return false if some error occurred
    virtual bool get_page( PageCachePage * pg ) = 0;
    virtual bool put_page( PageCachePage * pg ) = 0;
};

// following is an example of how to create a PageIO object.
// this one is useful for local files.  just open(2) the file
// and pass the fd to this class.

class PageIOFileDescriptor : public PageIO {
    int fd;
public:
    PageIOFileDescriptor(int _fd) { fd = _fd; }
    // note destructor does NOT close fd!
    ~PageIOFileDescriptor(void) { /* nothing */ }
    // return false if some error occurred
    bool get_page( PageCachePage * pg ) {
        lseek(fd, pg->get_page_number() * PageCache::PAGE_SIZE, SEEK_SET);
        if (read(fd, pg->get_ptr(), PageCache::PAGE_SIZE) < 0)
            return false;
        return true;
    }
    bool put_page( PageCachePage * pg ) {
        lseek(fd, pg->get_page_number() * PageCache::PAGE_SIZE, SEEK_SET);
        if (write(fd, pg->get_ptr(),
                  PageCache::PAGE_SIZE) != PageCache::PAGE_SIZE)
            return false;
        return true;
    }

};

#endif /* __PAGE_CACHE_H__ */
