
#include "dll2.H"
#include <stdlib.h>
#include <stdio.h>

enum { PAGE_LIST, PAGE_HASH, PAGE_LRU_LOCK, NUM_PAGE_LISTS };

class PCPInt : public PageCachePage {
    int refcount;
    friend class PageCachePageList;
    // only PageCachePageList can change reference counts on
    // this object.
    int ref(void) { return ++refcount; }
    int deref(void) {
        if (refcount == 0)
        {
            fprintf(stderr, "error, dereferencing a page already at count=0\n");
            exit(1);
        }
        return --refcount;
    }
public:
    LListLinks<PCPInt>  links[NUM_PAGE_LISTS];
    PCPInt(int _page_number) : PageCachePage(_page_number) { refcount = 0; }
    bool is_locked(void) { return (refcount != 0); }
};

class PCPIntHashCompare {
public:
    static int hash_key( PCPInt * item ) { return item->get_page_number(); }
    static int hash_key( int key ) { return key; }
    static bool hash_key_compare( PCPInt * item, int key ) {
        return (item->get_page_number() == key);
    }
};

class PageCachePageList {
    LList     <PCPInt,                      PAGE_LIST>  list;
    LListHash <PCPInt,int,PCPIntHashCompare,PAGE_HASH>  hash;
    LListLRU  <PCPInt,                  PAGE_LRU_LOCK>  lru;
    LList     <PCPInt,                  PAGE_LRU_LOCK>  locklist;
public:
    int get_cnt    (void) { return list.get_cnt(); }
    int get_lru_cnt(void) { return lru .get_cnt(); }
    PCPInt * find( int page_number ) { return hash.find( page_number ); }
    PCPInt * get_head  (void      ) { return list.get_head ( ); }
    PCPInt * get_next  (PCPInt * i) { return list.get_next (i); }
    PCPInt * get_oldest(void      ) { return lru.get_oldest( ); }
    void add( PCPInt * p, bool locked ) { 
        list.add(p);
        hash.add(p);
        if (locked) {
            locklist.add(p);
            if (p->ref() != 1) {
                fprintf(stderr, "PageCachePageList::add: inconsistent lock!\n");
                exit( 1 );
            }
        } else
            lru.add(p);
    }
    void remove( PCPInt * p ) {
        list.remove(p);
        hash.remove(p);
        if (p->is_locked())
            locklist.remove(p);
        else
            lru.remove(p);
    }
    void ref( PCPInt * p ) {
        if (p->ref() == 1) {
            lru.remove(p);
            locklist.add(p);
        }
    }
    void deref( PCPInt * p ) {
        if (p->deref() == 0) {
            locklist.remove(p);
            lru.add(p);
        }
    }
};
