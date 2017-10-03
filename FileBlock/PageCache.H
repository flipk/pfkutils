
#ifndef __PAGE_CACHE_H__
#define __PAGE_CACHE_H__

#include "types.H"

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
    PageCachePage * get(int page_number);
    void unlock( PageCachePage * p, bool dirty );
    void flush(void);
};

class PageCachePage {
    int page_number;
public:
    PageCachePage(int _page_number) {
        dirty = false;  page_number = _page_number;
        ptr = new UCHAR[PageCache::PAGE_SIZE];
    }
    int get_page_number(void) { return page_number; }
    ~PageCachePage(void) { delete[] ptr; }
    UCHAR * ptr;
    bool dirty;
};

class PageIO {
public:
    virtual ~PageIO(void) { /* placeholder */ }
    // return false if some error occurred
    virtual bool get_page( PageCachePage * pg ) = 0;
    virtual bool put_page( PageCachePage * pg ) = 0;
};

#endif /* __PAGE_CACHE_H__ */
