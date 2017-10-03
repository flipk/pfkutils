
#ifndef __RECNOBLOCK_H__
#define __RECNOBLOCK_H__

#include <time.h>
#include "dll2.H"
#include "types.H"

enum { BT_DLL2_LRU,  BT_DLL2_HASH,  BT_DLL2_EXTRA, BT_DLL2_COUNT };

#ifndef FBN_DEFAULT_RECORD_SIZE
#define FBN_DEFAULT_RECORD_SIZE    16
#endif

#ifndef FBN_DEFAULT_PAGE_SIZE
#define FBN_DEFAULT_PAGE_SIZE    4096
#endif

class FileBlockNumber {
    int        recordsize;    // number of bytes in a record
    int          pagesize;    // number of bytes in a page
    int       segmentsize;
    int pages_per_segment;
    int     recs_per_page;
    int  recs_per_segment;
    int     reserved_bits;

    static const int   block_signature = 0x504b;  // "PK"
    static const int  user_buffer_flag = 1;

    // cache parameters
    static const int         sync_time =  3;       // in seconds
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
private:
    void init( char * file, int h1, int h2, int c1,
                     int _record_size, int _page_size )
        throw ( constructor_failed );
public:
    static const int     size_overhead = 4;    // two for sig, two for size
    FileBlockNumber( char * file, int h1, int h2, int c1,
                     int _record_size, int _page_size )
        throw ( constructor_failed );
    FileBlockNumber( char * file, int h1, int h2, int c1 )
        throw ( constructor_failed );
    ~FileBlockNumber( void );
    int min_block_no( void ) { return reserved_bits; }
    // max block size determined by size of "szfield" in first record
    // (16 bit value) as well as how many bytes used up by magic and
    // size info
    int max_block_size( void ) { return (65536 - size_overhead); }
    int alloc( int bytes );   // return a blockno
    void free( int blockno );
    UCHAR * get_block( int blockno, int *size, UINT32 *magic );
    UCHAR * get_block( int blockno, UINT32 *magic ) {
        return get_block( blockno, NULL, magic );
    }
    void unlock_block( UINT32 magic, bool dirty );
    void put_block( int blockno, UCHAR * buf, int size );
    // returns number of pages written
    int flush( void );

    void cache_info( int * bitmap_pages, int * data_pages, int * dirty );
    void file_info( int * num_segments, int * recs_in_use, int * recs_free );
};

#endif
