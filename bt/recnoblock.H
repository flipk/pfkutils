
#ifndef __RECNOBLOCK_H__
#define __RECNOBLOCK_H__

// 
// the 6 args to the constructor of this class are:
//    char * file
//    int c1            // maximum number of data pages held in memory
//    int _record_size  // bytes in record, default=16
//    int _page_size    // bytes in page, default=4096
//

#include <time.h>
#include "dll2.H"
#include "types.H"

enum { BT_DLL2_LRU,  BT_DLL2_HASH,  BT_DLL2_EXTRA, BT_DLL2_COUNT };

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

    // cache parameters in seconds
    static const int         sync_time =  3;
    static const int           max_age = 15;

    // private structures
    struct page;
    struct FBN_page_hash_1;
    typedef LListHashLRU <page,UINT32,FBN_page_hash_1,
                          BT_DLL2_HASH,BT_DLL2_LRU> page_lru;
    typedef LList<page,BT_DLL2_EXTRA> extra_list;
    page_lru bitmaps;    // key is segment number
    page_lru data;       // key is page number

    int fd;
    int maxpages;
    time_t last_sync;

    page * get_data_page( int num, bool for_write );
    page * get_segment_bitmap( int num );
    void release_page( page * );
    int recs_from_size( int size ) {
        int recs = (size+size_overhead) / recordsize;
        if (((size+size_overhead)%recordsize) != 0)
            recs++;
        return recs;
    }
    UCHAR * _get_block( UINT32 blockno, int *size,
                        UINT32 *magic, bool for_write, bool do_error );

    static int compare( page ** a, page ** b );

public:
    class constructor_failed { };
private:
    void init( char * file, int c1,
                     int _record_size, int _page_size )
        throw ( constructor_failed );
public:
    static const int     size_overhead = 4;    // two for sig, two for size
    FileBlockNumber( char * file, int c1,
                     int _record_size, int _page_size )
        throw ( constructor_failed );
    FileBlockNumber( char * file, int c1 )
        throw ( constructor_failed );
    ~FileBlockNumber( void );
    int min_block_no( void ) { return reserved_bits; }
    // max block size determined by size of "szfield" in first record
    // (16 bit value) as well as how many bytes used up by magic and
    // size info
    int max_block_size( void ) { return (65536 - size_overhead); }
    UINT32 alloc( int bytes );   // return a blockno
    void free( UINT32 blockno );
    UCHAR * __get_block( UINT32 blockno, int *size, UINT32 *magic ) {
        return _get_block( blockno, size, magic, false, false );
    }
    UCHAR * get_block( UINT32 blockno, int *size, UINT32 *magic ) {
        return _get_block( blockno, size, magic, false, true );
    }
    UCHAR * get_block( UINT32 blockno, UINT32 *magic ) {
        return _get_block( blockno, NULL, magic, false, true );
    }
    UCHAR * get_block_for_write( UINT32 blockno, int *size, UINT32 *magic ) {
        return _get_block( blockno, size, magic, true, true );
    }
    void unlock_block( UINT32 magic, bool dirty );
    // returns number of pages written
    int flush( void );

    // return some statistics
    void cache_info( int * bitmap_pages, int * data_pages, int * dirty );
    void file_info( int * num_segments, int * recs_in_use, int * recs_free );
};

#ifdef RECNOBLOCK_INTERNAL


#if DLL2_INCLUDE_LOGNEW
#include "lognew.H"
#else
#define LOGNEW new
#endif

struct FileBlockNumber :: page {
    LListLinks<FileBlockNumber::page>  links[ BT_DLL2_COUNT ];

    // if this page describes a bitmap page, then the hash key_value
    // is the segment number.  if this page is a data page, the key_value
    // describes the page number (same as pagenum field).

    UINT32 key_value;
    int refcount;
    time_t reftime;
    UINT32 pagenum;
    bool dirty;
    UINT32 _buf[ 0 ];
//
    UCHAR * bufp( void ) { return (UCHAR*) _buf; }
//
    static void * operator new( size_t sz, int pagesize ) {
        char * ptr = new char[ sizeof(page) + pagesize ];
        return (void*) ptr;
    }
    static void operator delete( void * ptr ) {
        char * p = (char*) ptr;
        delete[] p;
    }
    // constructor that goes off and reads in the page from disk
    page( UINT32 pgnum, UINT32 key, int fd, int pagesize ) {
        init( pgnum, key, fd, pagesize );
    }
    // constructor that does not read from disk--
    // just initializes the space
    page( UINT32 pgnum, UINT32 key, int pagesize ) {
        init2( pgnum, key, pagesize );
    }
    ~page( void ) {
        if ( dirty )
        {
            fprintf( stderr, "deleting a dirty page!\n" );
            kill(0,6);
        }
    }
    void init( UINT32 _pgnum, UINT32 _key, int fd, int pagesize ) {
        pagenum = _pgnum; key_value = _key;
        dirty = false; refcount = 0;
        lseek( fd, (off_t)pagenum*(off_t)pagesize, SEEK_SET );
        int cc = read( fd, bufp(), pagesize );
        if ( cc < 0 ) {
            fprintf( stderr, "read file failed: %s\n", strerror( errno ));
            kill(0,6); }
        if ( cc != pagesize )
            memset( bufp()+cc, 0, pagesize-cc );
        time( &reftime );
    }
    void init2( UINT32 _pgnum, int _key, int pagesize ) {
        pagenum = _pgnum; key_value = _key;
        dirty = true; refcount = 0; // assume dirty!
        memset( bufp(), 0, pagesize );
        time( &reftime );
    }
    bool clean( int fd, int pagesize ) {
        if ( dirty )
        {
            lseek( fd, (off_t)pagenum*(off_t)pagesize, SEEK_SET );
            write( fd, bufp(), pagesize );
            dirty = false;
            return true;
        }
        return false;
    }
    int age(void) { return time(0) - reftime; }
    bool   ref( void ) {
        time( &reftime );
        return (refcount++ == 0); // true if first ref
    }
    bool deref( void ) {
        if ( refcount == 0 )
            kill( 0, 6 );
        return (--refcount == 0);
    } // true if last deref

// this method is used only if this page is a data page
    UCHAR * getrecord( int rec, int recordsize ) {
        return bufp() + (rec*recordsize);
    }

// these methods are used only if this page is a segment bitmap page
    void   setbit ( int bit ) {
        _buf[ bit / 32 ] |=  (1 << (bit % 32));
    }
    void clearbit ( int bit ) {
        _buf[ bit / 32 ] &= ~(1 << (bit % 32));
    }
    bool   getbit ( int bit ) {  // return true if set
        return (_buf[ bit / 32 ] & (1 << (bit % 32))) != 0;
    }
    int countbits ( int bit, int max, int pagesize ) {
        int cnt = 1;
        bool v = getbit(bit++);
        for ( ; bit < (pagesize*8) && cnt <= max; )
        {
            bool jump32 = false;
            if (( bit & 31 ) == 0 )
            {
                if ( v )
                {
                    if (_buf[ bit / 32 ] == ~0U)
                        jump32 = true;
                }
                else
                {
                    if (_buf[ bit / 32 ] ==  0)
                        jump32 = true;
                }
                if ( jump32 )
                {
                    bit += 32;
                    cnt += 32;
                    continue;
                }
            }
            if ( getbit(bit) != v )
                break;
            else
            {
                bit++; cnt++;
            }
        }
        return cnt;
    }
};

class FileBlockNumber :: FBN_page_hash_1 {
public:
    static int hash_key( FileBlockNumber :: page * item ) {
        return (int) item->key_value;
    }
    static int hash_key( UINT32 key ) {
        return (int) key;
    }
    static bool hash_key_compare( FileBlockNumber :: page * item,
                                  UINT32 key ) {
        return (item->key_value == key);
    }
};


// use one of these during a 'get_block' if the block
// crosses 1 or more page boundaries and the memory
// can't be provided contiguously.  the 'magic' will
// have a special bit set if this is the case.

struct user_buffer {
    UINT32 blockno;
    int size;
    UCHAR data[0];
//
    user_buffer( UINT32 _blk ) {
        blockno = _blk;
    }
    void * operator new( size_t s, int sz, char *file, int line ) {
        int alloc_size =
            sz + sizeof( user_buffer ) + FileBlockNumber::size_overhead;
#if DLL2_INCLUDE_LOGNEW
        char * ret = new(file,line) char[ alloc_size ];
#else
        char * ret = new char[ alloc_size ];
#endif
        ((user_buffer*)ret)->size = sz;
        return (void*)ret;
    }
    void operator delete( void * ptr ) {
        char * buf = (char*)ptr;
        delete[] buf;
    }
};

#endif /* RECNOBLOCK_INTERNAL */

#endif /* __RECNOBLOCK_H__ */
