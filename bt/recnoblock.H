
#ifndef __RECNOBLOCK_H__
#define __RECNOBLOCK_H__

#include <time.h>
#include "dll2.H"
#include "types.H"

enum { BT_DLL2_LRU,  BT_DLL2_HASH,  BT_DLL2_EXTRA, BT_DLL2_COUNT };

class FileBlockNumber {
    static const int        recordsize = 16;
    static const int          pagesize = 4096;
    static const int     size_overhead = 4;
    static const int   block_signature = 0x504b;  // "PK"
    static const int  user_buffer_flag = 0x80000000;
    static const int         sync_time = 3;       // in seconds
    static const int       segmentsize = pagesize    * recordsize * 8;
    static const int pages_per_segment = segmentsize / pagesize;
    static const int     recs_per_page = pagesize    / recordsize;
    static const int  recs_per_segment = segmentsize / recordsize;
    static const int     reserved_bits = pagesize    / recordsize;
    static const int           max_age = 15;  // in seconds

    struct page;         // private
    struct FBN_page_hash_1;  // private
    typedef LListHashLRU <page,int,FBN_page_hash_1,
                          BT_DLL2_HASH,BT_DLL2_LRU> page_lru;
    typedef LList<page,BT_DLL2_EXTRA> extra_list;
    page_lru bitmaps;    // key is segment number
    page_lru data;       // key is page number

    int fd;
    int maxpages;
    time_t last_sync;

    page * get_data_page( int num );
    page * get_segment_bitmap( int num );
    void release_page( page * );
    int recs_from_size( int size ) {
        int recs = (size+size_overhead) / recordsize;
        if (((size+size_overhead)%recordsize) != 0)
            recs++;
        return recs;
    }

    static int compare( page ** a, page ** b );

public:
    class constructor_failed { };
    FileBlockNumber( char * file, int h1, int h2, int c1 )
        throw ( constructor_failed );
    ~FileBlockNumber( void );
    static const int min_block_no = reserved_bits;
    int alloc( int bytes );   // return a blockno
    void free( int blockno );
    UCHAR * get_block( int blockno, int &size, UINT32 &magic );
    UCHAR * get_block( int blockno, UINT32 &magic );
    void unlock_block( UINT32 magic, bool dirty );
    void flush( void );
};

#endif
