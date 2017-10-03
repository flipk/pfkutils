
// a 'record' is 16 bytes.
// a 'block' is a set of contiguous records.
// a 'page' is 4k (256 records).
// a 'segment' is 512k (128 pages, 32768 records).
// a 'bitmap page' is the first page of a segment, containing free-space bits
// a 'data page' is the other 127 pages of a segment, containing data
// 
// since the bitmap takes up the first 256 records of the segment,
// the first 256 bits (32 bytes, 2 records) of the bitmap page are
// not bits, instead they are other information, such as number of
// records in use in that segment, a 'magic' to identify the
// segment, etc.
// 
// bitmap pages are stored in a separate cache from the data pages.

//
// TODO:
//    add a method for get_block_for_write which does not access the
//    disk to read first-- assumes the user doesn't actually want to
//    read the contents and only wants a new space.  alternatively
//    perhaps a put_block function which will take a user pointer/len
//    and block number and does the write in one step.
//
//    add an option to write updates to a separate file as a journal,
//    for rollback purposes-- each 'commit' call results in all updates
//    stored in the journal being copied back to the file.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "recnoblock.H"

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

    int key_value;
    int refcount;
    time_t reftime;
    int pagenum;
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
    page( int pgnum, int key, int fd, int pagesize ) {
        init( pgnum, key, fd, pagesize );
    }
    ~page( void ) {
        if ( dirty )
        {
            fprintf( stderr, "deleting a dirty page!\n" );
            kill(0,6);
        }
    }
    void init( int _pgnum, int _key, int fd, int pagesize ) {
        pagenum = _pgnum; key_value = _key;
        dirty = false; refcount = 0;
        lseek( fd, pagenum*pagesize, SEEK_SET );
        int cc = read( fd, bufp(), pagesize );
        if ( cc < 0 ) {
            fprintf( stderr, "read file failed: %s\n", strerror( errno ));
            kill(0,6); }
        if ( cc != pagesize )
            memset( bufp()+cc, 0, pagesize-cc );
        time( &reftime );
    }
    bool clean( int fd, int pagesize ) {
        if ( dirty )
        {
            lseek( fd, pagenum*pagesize, SEEK_SET );
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
                    if (_buf[ bit / 32 ] == ~0)
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
        return item->key_value;
    }
    static int hash_key( int key ) {
        return key;
    }
    static bool hash_key_compare( FileBlockNumber :: page * item,
                                  int key ) {
        return (item->key_value == key);
    }
};


// use one of these during a 'get_block' if the block
// crosses 1 or more page boundaries and the memory
// can't be provided contiguously.  the 'magic' will
// have a special bit set if this is the case.

struct user_buffer {
    int blockno;
    int size;
    UCHAR data[0];
//
    user_buffer( int _blk ) {
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

FileBlockNumber :: FileBlockNumber( char * file, int h1, int h2, int c1,
                                    int _record_size, int _page_size )
    throw ( constructor_failed )
    : bitmaps( h2 ), data( h1 )
{
    init( file, h1, h2, c1, _record_size, _page_size );
}

FileBlockNumber :: FileBlockNumber( char * file, int h1, int h2, int c1 )
    throw ( constructor_failed )
    : bitmaps( h2 ), data( h1 )
{
    init( file, h1, h2, c1, FBN_DEFAULT_RECORD_SIZE, FBN_DEFAULT_PAGE_SIZE );
}

static bool
power_of_2( int value )
{
    if ((( value ^ (value-1)) + 1) != (value << 1))
        return false;
    return true;
}

void
FileBlockNumber :: init( char * file, int h1, int h2, int c1,
                         int _record_size, int _page_size )
    throw ( constructor_failed )
{
    recordsize = _record_size;
    pagesize   = _page_size;

    if ( !power_of_2( pagesize ) || !power_of_2( recordsize ))
    {
        fprintf( stderr, "pagesize %d and recordsize %d "
                 "must both be a power of 2\n", pagesize, recordsize );
        kill(0,6);
    }

    if (( pagesize % recordsize ) != 0 )
    {
        fprintf( stderr, "pagesize %d must be a multiple of recordsize %d\n",
                 pagesize, recordsize );
        kill(0,6);
    }

    // each bit in the bitmap page represents a record; a page
    // has pagesize*8 bits so '(pagesize*8)*recordsize' is the
    // size of a segment.

    segmentsize = recordsize * pagesize * 8;

    // any data file will always be 1/n bitmap, where 'n' is the ratio
    // between the size of a record and the size of a bit.  thus for each
    // 1 bitmap page, there will be n-1 data pages in a segment, and 'n'
    // is determined by the number of bits in a record.

    pages_per_segment = recordsize * 8;
    recs_per_page = pagesize / recordsize;
    recs_per_segment = segmentsize / recordsize;

    // the first 'n' bits of a bitmap page will correspond to the bitmap
    // page itself.  calculate how many bits are reserved.

    reserved_bits = pagesize / recordsize;

    maxpages = c1;
    fd = open( file, O_RDWR );
    if ( fd < 0 )
    {
        fd = open( file, O_RDWR | O_CREAT, 0644 );
        if ( fd < 0 )
            throw constructor_failed();
    }
    time( &last_sync );

    // note this code will not work on a platform where a pointer
    // is 64 bits.  haven't hit that case yet but someday I will.

    if ( sizeof(void*) != sizeof(UINT32) )
    {
        fprintf( stderr, "error: FileBlockNumber assumes pointers are "
                 "32-bit values!\n" );
        kill(0,6);
    }
}

FileBlockNumber :: ~FileBlockNumber( void )
{
    page * p, * np;
    for ( p = bitmaps.get_lru_head(); p; p = np )
    {
        np = bitmaps.get_lru_next(p);
        bitmaps.remove( p );
        p->clean( fd, pagesize );
        delete p;
    }
    for ( p = data.get_lru_head(); p; p = np )
    {
        np = data.get_lru_next(p);
        data.remove( p );
        p->clean( fd, pagesize );
        delete p;
    }
    close( fd );
}

// we place a limit on caching data pages in the data hash/lru.
// we do not limit bitmap pages.

FileBlockNumber::page *
FileBlockNumber :: get_data_page( int page_num )
{
    page * ret;

    ret = data.find( page_num );
    if ( ret != NULL )
    {
        ret->ref();
        return ret;
    }

    // note 'ret' is null here

    extra_list dellist;
    page * candidate;

    for ( candidate = data.get_lru_head();
          candidate;
          candidate = data.get_lru_next(candidate) )
    {
        if ( ( data.get_cnt() - dellist.get_cnt() ) < maxpages )
            break;

        if ( candidate->refcount > 0 )
            // can't delete this one, so skip it
            continue;

        dellist.add( candidate );
    }

    while ( candidate = dellist.get_head() )
    {
        dellist.remove( candidate );
        data.remove( candidate );

        // careful of this logic, what its doing is 
        // allowing us to reuse the last candidate
        // so we don't have to delete and then new
        // the same type.

        if ( ret != NULL )
        {
            ret->clean( fd, pagesize );
            delete ret;
        }
        ret = candidate;
    }

    // if we're reusing, then clean/init. else, use new.

    if ( ret != NULL )
    {
        ret->clean( fd, pagesize );
        ret->init( page_num, page_num, fd, pagesize );
    }
    else
    {
        ret = new(pagesize) page( page_num, page_num, fd, pagesize );
    }

    ret->ref();
    data.add( ret );

    return ret;
}

FileBlockNumber::page *
FileBlockNumber :: get_segment_bitmap( int seg_num )
{
    page * ret = bitmaps.find( seg_num );
    if ( ret == NULL )
    {
        ret = new(pagesize)
            page( seg_num * pages_per_segment, seg_num, fd, pagesize );
        bitmaps.add( ret );
    }
    ret->ref();
    return ret;
}

void
FileBlockNumber :: release_page ( page * p )
{
    time_t now;
    p->deref();
    time( &now );
    if (( now - last_sync ) >= sync_time )
        flush();
}

UCHAR *
FileBlockNumber :: get_block( int blockno, int *sizep, UINT32 *magic )
{
    // which file segment is this block in?
    int seg_num        = blockno / recs_per_segment;
    // which file page is this block in?
    int pag_num_in_fil = blockno / recs_per_page;
    // which record in the segment is this?
    int rec_in_seg     = blockno - seg_num        * recs_per_segment;
    // which record in this page is it?
    int rec_in_pag     = blockno - pag_num_in_fil * recs_per_page;

    page *    bm = get_segment_bitmap( seg_num );
    if ( !bm->getbit( rec_in_seg ))
    {
        fprintf( stderr, "error, block %d marked as free!\n", blockno );
        kill(0,6);
    }
    release_page( bm );

    page * datpg = get_data_page( pag_num_in_fil );

    UCHAR * ret = datpg->getrecord( rec_in_pag, recordsize );
    UINT16_t * sig     = (UINT16_t *)ret;

    if ( sig->get() != block_signature )
    {
        fprintf( stderr, "error in get, block %d "
                 "does not have signature!\n", blockno );
        kill(0,6);
    }

    UINT16_t * szfield = (UINT16_t *)(ret + 2);
    int size = szfield->get();
    if ( sizep )
        *sizep = size;

    int recs = recs_from_size( size );

    if (( rec_in_pag + recs ) < recs_per_page )
    {
        *magic = (UINT32) datpg;
        return ret + size_overhead;
    }

    // else it crosses a page boundary and we need to
    // return a user_buffer instead.

    user_buffer * ub = new( size, __FILE__, __LINE__ ) user_buffer( blockno );

    int left = size + size_overhead;
    UCHAR * dst = ub->data;
    bool first = true;

    // populate ub
    while ( left > 0 )
    {
        int tocopy;
        UCHAR * src;
        if ( !first )
            datpg = get_data_page( pag_num_in_fil );
        pag_num_in_fil++;
        first = false;

        // get next datapage, setup src / tocopy
        src = datpg->getrecord( rec_in_pag, recordsize );
        rec_in_pag = 0;
        tocopy = (datpg->bufp() + pagesize) - src;
        if ( tocopy > left )
            tocopy = left;

        memcpy( dst, src, tocopy );
        dst += tocopy;
        left -= tocopy;

        // release data page
        release_page( datpg );
    }

    *magic = (UINT32) ub  | user_buffer_flag;

    // note that the user buffer contains the sig and size info too
    // though the pointer we return to the caller skips that.

    return ub->data + size_overhead;
}

// copy src to dest, for 'tocopy' bytes; 
// return false if the two memory segments are different
// or true if they were the same.

static bool
memcpydiff( UCHAR * dest, UCHAR * src, int len )
{
    while ( len > 0 )
    {
        if ( *dest != *src )
        {
            // short circuit and memcpy the rest (efficient)
            // and drop out of the while loop.

            memcpy( dest, src, len );
            return false;
        }
        dest++, src++, len--;
    }
    return true;
}

void
FileBlockNumber :: unlock_block( UINT32 magic, bool dirty )
{
    if ( magic & user_buffer_flag )
    {
        user_buffer * ub = (user_buffer *)( magic & ~user_buffer_flag );

        int blockno = ub->blockno;
        
        if ( dirty )
        {
            // sync ub back into pages
            int pag_num_in_fil = blockno / recs_per_page;
            int rec_in_pag = blockno - pag_num_in_fil * recs_per_page;

            int left = ub->size + size_overhead;
            UCHAR * src = ub->data;

            while ( left > 0 )
            {
                int tocopy;
                UCHAR * dest;
                page * datpg;

                // note that the user buffer contains the sig and size
                // so it is okay to copy all the records without 
                // special-casing the first one.

                datpg = get_data_page( pag_num_in_fil );
                pag_num_in_fil++;
                dest = datpg->getrecord( rec_in_pag, recordsize );
                rec_in_pag = 0;
                tocopy = (datpg->bufp() + pagesize) - dest;
                if ( tocopy > left )
                    tocopy = left;

                // only mark this page as dirty if this page changed.
                // oh, and if it changed, update its contents.
                // a helper function written to do both at once.

                if ( memcpydiff( dest, src, tocopy ) == false )
                    datpg->dirty = true;

                src += tocopy;
                left -= tocopy;

                release_page( datpg );
            }
        }

        delete ub;
    }
    else
    {
        page * p = (page *)magic;
        p->deref();
        if ( dirty )
            p->dirty = true;
    }
}

void
FileBlockNumber :: put_block( int blockno, UCHAR * buf, int size )
{
    fprintf( stderr, "FileBlockNumber :: put_block : xxx unimplemented\n" );
    kill(0,6);
}

int
FileBlockNumber :: alloc( int bytes )
{
    if ( bytes > max_block_size() )
    {
        fprintf( stderr, "allocate %d bytes: not allowed (max %d)\n",
                 bytes, max_block_size() );
        kill(0,6);
    }

    page * bm;
    int blockno, seg_num, bit, bitstart, recs, cnt, largest_seg;

    recs = recs_from_size( bytes );

    while ( 1 )
    {
        largest_seg = -1;
        for ( cnt = bitmaps.get_cnt(); cnt >= 0; cnt-- )
        {
            bm = bitmaps.get_lru_head();
            if ( !bm )
                break;

            if ( bm->key_value > largest_seg )
                largest_seg = bm->key_value;

            for ( bit = reserved_bits; bit < recs_per_segment; )
            {
                bool v = bm->getbit( bit );
                int c = bm->countbits( bit, recs, pagesize );

                if ( !v && c >= recs )
                {
                    seg_num = bm->key_value;
                    bitstart = bit;
                    bm->ref();
                    goto found;
                }
                bit += c;
            }

            // this one's no good, put it at the end of the list.
            bitmaps.promote( bm );
        }

        // ran thru the entire page list and couldn't find
        // the space. time to grow the file.  conveniently,
        // largest_seg tells us where the largest segment is.
        // note that if this process opened an existing file,
        // this will instead walk thru the existing bitmaps until either
        // space is found or a new segment is created.
        get_segment_bitmap( largest_seg+1 )->deref();
    }

 found:

    // 'seg_num' indicates the segment with free space
    // 'bitstart' indicates the rec within that segment
    // 'bm' still points to that bitmap

    blockno = seg_num * recs_per_segment + bitstart;

    int pag_num_in_fil = blockno / recs_per_page;
    int rec_in_pag     = blockno - pag_num_in_fil * recs_per_page;

    for ( bit = bitstart; recs > 0; bit++, recs-- )
        bm->setbit( bit );
    bm->dirty = true;
    release_page( bm );

    UCHAR * recptr;
    UINT16_t * mag;
    UINT16_t * sz;

    page * dat = get_data_page( pag_num_in_fil );

    recptr = dat->getrecord( rec_in_pag, recordsize );
    mag = (UINT16_t*) recptr;
    sz  = (UINT16_t*)(recptr+2);

    mag->set( block_signature );
    sz->set( bytes );
    dat->dirty = true;
    release_page( dat );

    return blockno;
}

void
FileBlockNumber :: free( int blockno )
{
    int seg_num        = blockno / recs_per_segment;
    int pag_num_in_fil = blockno / recs_per_page;
    int rec_in_seg     = blockno - seg_num        * recs_per_segment;
    int rec_in_pag     = blockno - pag_num_in_fil * recs_per_page;

    page * bm = get_segment_bitmap( seg_num );
    if ( !bm->getbit( rec_in_seg ))
    {
        fprintf( stderr,
                 "error, block %d is already marked as free!\n", blockno );
        kill(0,6);
    }
    page * datpg = get_data_page( pag_num_in_fil );

    UCHAR * recptr = datpg->getrecord( rec_in_pag, recordsize );
    UINT16_t * sig = (UINT16_t*)recptr;
    UINT16_t * szfld = (UINT16_t*)(recptr+2);
    int size = szfld->get();

    if ( sig->get() != block_signature )
    {
        fprintf( stderr, "error in free, block %d does "
                 "not have signature!\n", blockno );
        kill(0,6);
    }
    sig->set( 0xeeee );
    szfld->set( 0xd0d0 );
    datpg->dirty = true;
    release_page( datpg );

    int recs = recs_from_size( size );

    int bit;
    for ( bit = rec_in_seg; recs > 0; bit++,recs-- )
        bm->clearbit( bit );
    bm->dirty = true;
    release_page( bm );
}

//static
int
FileBlockNumber :: compare( page ** a, page ** b )
{
    if ( (*a)->pagenum > (*b)->pagenum )
        return 1;
    if ( (*a)->pagenum < (*b)->pagenum )
        return -1;
    return 0;
}

int
FileBlockNumber :: flush( void )
{
    page * p, ** pgs;
    int i, len, written = 0;

    pgs = LOGNEW page*[ bitmaps.get_cnt() + data.get_cnt() ];
    len = 0;

    for ( p = bitmaps.get_lru_head(); p; p = bitmaps.get_lru_next(p) )
        pgs[len++] = p;
    for ( p = data.get_lru_head(); p; p = data.get_lru_next(p) )
        pgs[len++] = p;

    qsort( pgs, len, sizeof(page*),
           (int(*)(const void*,const void*))&FileBlockNumber::compare );

    for ( i = 0; i < len; i++ )
    {
        p = pgs[i];
        if ( p->clean( fd, pagesize ))
            written++;
        if ( p->refcount == 0 && p->age() > max_age )
        {
            if ( bitmaps.onthislist(p))
                bitmaps.remove( p );
            if ( data.onthislist(p))
                data.remove( p );
            delete p;
        }
    }

    delete[] pgs;
    time( &last_sync );
//    printf( "flush wrote %d pages\n", written );
    return written;
}

void
FileBlockNumber :: cache_info( int * bitmap_pages,
                               int * data_pages,
                               int * dirty )
{
    int count = 0;
    *bitmap_pages = bitmaps.get_cnt();
    *data_pages = data.get_cnt();
    page * p;
    for ( p = bitmaps.get_lru_head(); p; p = bitmaps.get_lru_next(p) )
        if ( p->dirty )
            count++;
    for ( p = data.get_lru_head(); p; p = data.get_lru_next(p) )
        if ( p->dirty )
            count++;
    *dirty = count;
}

void
FileBlockNumber :: file_info( int * _num_segments,
                              int * _recs_in_use, int * _recs_free )
{
    struct stat sb;
    fstat( fd, &sb );

    int num_segments = sb.st_size / segmentsize;
    if (( sb.st_size % segmentsize ) != 0 )
        num_segments++;

    page * p;
    int i, j, count0=0, count1=0;

    flush();

    p = new(pagesize) page( 0, 0, fd, pagesize );

    for ( i = 0; i < num_segments; i++ )
    {
        if ( i != 0 )
            p->init( i, i, fd, pagesize );
        for ( j = reserved_bits; j < (pagesize*8); j++ )
            if ( p->getbit(j) )
                count1++;
            else
                count0++;
    }
    delete p;

    *_num_segments = num_segments;
    *_recs_in_use = count1;
    *_recs_free = count0;
}


#ifdef INCLUDE_MAIN

#define DATSIZE   5000 // 64k or 5000 or 200
#define MAXDATS   5000
#define ROUNDS  100000

struct testdat {
    bool used;
    int blockno;
    int size;
    UCHAR data[ DATSIZE ];
};

testdat td[ MAXDATS ];

int reps;
int additions;
int deletions;
int modifications;

FILE * lf;

int
main()
{
    int size, v;
    UINT32 magic;
    UCHAR * buf;
    time_t start, stop, last, now;

    unlink( "00LOG" );
    lf = NULL;
//    lf = fopen( "00LOG", "w" );

    if ( lf ) setlinebuf( lf );
    int seed;

//    seed = getpid() * time(0);
    seed = 1695247224;

    srandom( seed );
    if ( lf )
        fprintf( lf, "S %d\n", seed );
    memset( &td, 0, sizeof( td ));
    unlink( "testdb" );
    FileBlockNumber f( "testdb", 100, 100, 100, 16, 4096 );

    time( &last );
    start = last;

    reps = 0;
    additions = 0;
    deletions = 0;
    modifications = 0;

    int r;
    for ( r = 0; r < ROUNDS; r++ )
    {
        int ind = random() % MAXDATS;
        testdat * t = &td[ind];

        time( &now );
        if ( last != now )
        {
            last = now;
            int bmp, dp, dirt;
            f.cache_info( &bmp, &dp, &dirt );
            printf( "iter %d reps %d additions %d "
                    "deletions %d modifications %d\n",
                    r, reps, additions, deletions, modifications );
            printf( "  cache: bm %d data %d dirty %d\n", bmp, dp, dirt );
            reps = 0;
            additions = 0;
            deletions = 0;
            modifications = 0;
        }

        if ( t->used )
        {
            buf = f.get_block( t->blockno, &size, &magic );
            if ( size != t->size )
                kill( 0, 6 );
            if ( memcmp( buf, t->data, size ) != 0 )
                kill( 0, 6 );

            v = random() % 100;
            if ( v > 63 )
            {
                t->data[ random() % size ] = random() & 0xff;
                memcpy( buf, t->data, size );
                f.unlock_block( magic, true );

                if ( lf )
                    fprintf( lf, "M %d ind %d\n", r, ind );
                modifications++;
            }
            else if ( v > 33 )
            {
                f.unlock_block( magic, false );
                if ( lf )
                    fprintf( lf, "V %d ind %d\n", r, ind );
            }
            else
            {
                f.unlock_block( magic, false );
                f.free( t->blockno );
                t->used = false;

                if ( lf )
                    fprintf( lf, "F %d ind %d\n", r, ind );
                deletions++;
            }
        }
        else
        {
            t->used = true;
            size = random() % sizeof( t->data );
            if ( size == 0 )
                size = 1;
            t->size = size;

            for ( int i = 0; i < size; i++ )
                t->data[i] = random() & 0xff;

            t->blockno = f.alloc( size );
            buf = f.get_block( t->blockno, &magic );
            memcpy( buf, t->data, size );
            f.unlock_block( magic, true );

            if ( lf )
                fprintf( lf, "A %d ind %d blockno %d size %d ub %d\n",
                         r, ind, t->blockno, t->size, magic & 1 );
            additions++;
        }
        reps++;
    }

    time( &stop );

    printf( "completed %d reps in %d seconds, %d reps/second\n",
            r, stop - start, r / (stop-start) );
    int num_segments, recs_in_use, recs_free;
    f.file_info( &num_segments, &recs_in_use, &recs_free );
    printf( "file info: segments %d recs_in_use %d recs_free %d\n",
            num_segments, recs_in_use, recs_free );
}

#endif
