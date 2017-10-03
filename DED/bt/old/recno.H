
#ifndef __RECNO_H__
#define __RECNO_H__

#include <stdio.h>
#include <string.h>
#include "types.H"

// 
// a file is broken into 16-byte records, and there's a cache.
// the "cache size" argument to the constructor is a count of 
// 16-byte records that can be cached in memory.
// 

class FileRecordNumber {
public:
    // exception thrown when constructor fails
    class constructor_failed { };
    int record_length;

    void dumpcache( void );

private:
    struct frn_record {
        struct frn_record *next;
        struct frn_record *prev;
        struct frn_record *lru_next;
        struct frn_record *lru_prev;
        int clean;
        int recno;
        UCHAR dirty:1;
        UCHAR locked:1;
        UCHAR dat[ 0 ];
        void * operator new( size_t s, int len ) {
            s = sizeof( frn_record ) + len;
            void * ret = (void*) new char[ s ];
            memset( ret, 0, s );
            return ret;
        }
        void operator delete( void * _p ) {
            char * p = (char*) _p; 
            delete[] p;
        }
        frn_record( int _recno ) {
            recno = _recno;
            locked = 1;
        }
    };
    FILE * fd;

    // this searches the hash first.

    frn_record * _get_record( int recno );

    // these access the file directly.

    int __put_record( frn_record *x );
    int __get_record( frn_record *x );

    // deal with LRU. cache_size managed by add/delete.

    frn_record * lru_head;  // remove from here
    frn_record * lru_tail;  // insert to here
    int cache_size;
    int max_cache_size;
    void add_lru( frn_record * );
    frn_record * get_lru( void );
    void delete_lru( frn_record * );

    // deal with hash table.

    frn_record ** datahash;
    int hashsize;
    int numhash;
    int h;
    int hash( int recno ) { return recno % hashsize; }
    void add_hash( frn_record * );
    void del_hash( frn_record * );
    frn_record * find_hash( int recno );

    int flushes;

public:
    FileRecordNumber( char * filename,
                      int recsize, int _hashsize,
                      int _max_cache_size ) throw ( constructor_failed );
    ~FileRecordNumber( void );
    int flush( void );

    // this returns a locked record,
    // and a pointer to the internal cache entry.
    UCHAR * get_record( int recno, UINT32& magic );
    // this is a shortcut for the above if you know
    // you're not going to need to read it first.
    UCHAR * get_empty_record( int recno, UINT32& magic );
    // use this when you're done with the record.
    // 'magic' is the returned magic # from the above 2 calls.
    void unlock_record( UINT32 magic, bool dirty );
};

#endif
