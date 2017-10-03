
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

// TODO:
//    add an option to write updates to a separate file as a journal,
//    for rollback purposes-- each 'commit' call results in all updates
//    stored in the journal being copied back to the file.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "recnoblock.H"

#if DLL2_INCLUDE_LOGNEW
#include "lognew.H"
#else
#define LOGNEW new
#endif


struct FileBlockNumber :: page {
    LListLinks<FileBlockNumber::page>  links[ BT_DLL2_COUNT ];
    int key_value;
    int refcount;
    int reftime;
    int fd;
    int pagenum;
    UCHAR buf[pagesize];
    bool dirty;
//
    page( int pgnum, int key, int fd ) { init( pgnum, key, fd ); }
    ~page( void ) { clean(); }
    void init( int _pgnum, int _key, int _fd ) {
        fd = _fd; pagenum = _pgnum; key_value = _key;
        dirty = false; refcount = 0;
        lseek( fd, pagenum*pagesize, SEEK_SET );
        int cc = read( fd, buf, pagesize );
        if ( cc < 0 ) {
            printf( "read file failed: %s\n", strerror( errno ));
            kill(0,6); }
        if ( cc != pagesize ) memset( buf+cc, 0, pagesize-cc );
        reftime = (int) time( NULL );
    }
    void clean( void ) {
        if ( dirty )
        {
            lseek( fd, pagenum*pagesize, SEEK_SET );
            write( fd, buf, pagesize );
            dirty = false;
        }
    }
    int age(void) { return (int)time(0) - reftime; }
    bool   ref( void ) {
        reftime = (int)time(0);
        return (refcount++ == 0); // true if first ref
    }
    bool deref( void ) {
        if ( refcount == 0 )
            kill( 0, 6 );
        return (--refcount == 0);
    } // true if last deref
//
    UCHAR * getrecord( int rec ) { return &buf[rec*recordsize]; }
//
    void   setbit ( int bit ) { buf[bit/8] |=  (1<<(bit%8)); }
    void clearbit ( int bit ) { buf[bit/8] &= ~(1<<(bit%8)); }
    bool   getbit ( int bit ) { return (buf[bit/8] & (1<<(bit%8)))!=0; }
    int countbits ( int bit ) {
        int c = 1;
        bool v = getbit(bit++);
        for ( ; bit < (pagesize*8); bit++,c++ )
            if ( getbit(bit) != v )
                break;
        return c;
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
// have the high bit set if this is the case.

struct user_buffer {
    int blockno;
    int size;
    UCHAR data[1];
//
    user_buffer( int _blk ) {
        blockno = _blk;
    }
    void * operator new( size_t s, int sz, char *file, int line ) {
#if DLL2_INCLUDE_LOGNEW
        char * ret = new(file,line) char[ sizeof( user_buffer ) + sz ];
#else
        char * ret = new char[ sizeof( user_buffer ) + sz ];
#endif
        ((user_buffer*)ret)->size = sz;
        return (void*)ret;
    }
    void operator delete( void * ptr ) {
        char * buf = (char*)ptr;
        delete[] buf;
    }
};

FileBlockNumber :: FileBlockNumber( char * file, int h1, int h2, int c1 )
    throw ( constructor_failed )
    : bitmaps( h2 ), data( h1 )
{
    maxpages = c1;
    fd = open( file, O_RDWR );
    if ( fd < 0 )
    {
        fd = open( file, O_RDWR | O_CREAT, 0644 );
        if ( fd < 0 )
            throw constructor_failed();
    }
    time( &last_sync );
}

FileBlockNumber :: ~FileBlockNumber( void )
{
    page * p, * np;
    for ( p = bitmaps.get_lru_head(); p; p = np )
    {
        np = bitmaps.get_lru_next(p);
        bitmaps.remove( p );
        delete p;
    }
    for ( p = data.get_lru_head(); p; p = np )
    {
        np = data.get_lru_next(p);
        data.remove( p );
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
            delete ret;
        ret = candidate;
    }

    // if we're reusing, then clean/init. else, use new.

    if ( ret != NULL )
    {
        ret->clean();
        ret->init( page_num, page_num, fd );
    }
    else
    {
        ret = LOGNEW page( page_num, page_num, fd );
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
        ret = LOGNEW page( seg_num * pages_per_segment, seg_num, fd );
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
FileBlockNumber :: get_block( int blockno, int &size, UINT32 &magic )
{
    int seg_num        = blockno / recs_per_segment;
    int pag_num_in_fil = blockno / recs_per_page;
    int rec_in_seg     = blockno - seg_num        * recs_per_segment;
    int rec_in_pag     = blockno - pag_num_in_fil * recs_per_page;

    page *    bm = get_segment_bitmap( seg_num );
    if ( !bm->getbit( rec_in_seg ))
    {
        printf( "error, block %d marked as free!\n", blockno );
        kill(0,6);
    }
    release_page( bm );

    page * datpg = get_data_page( pag_num_in_fil );

    UCHAR * ret = datpg->getrecord( rec_in_pag );
    UINT16_t * sig     = (UINT16_t *)ret;
    UINT16_t * szfield = (UINT16_t *)(ret + 2);
    size = szfield->get();

    if ( sig->get() != block_signature )
    {
        printf( "error in get, block %d "
                "does not have signature!\n", blockno );
        kill(0,6);
    }

    int recs = recs_from_size( size );

    if (( rec_in_pag + recs ) < recs_per_page )
    {
        magic = (UINT32) datpg;
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
        src = datpg->getrecord( rec_in_pag );
        rec_in_pag = 0;
        tocopy = &datpg->buf[pagesize] - src;
        if ( tocopy > left )
            tocopy = left;

        memcpy( dst, src, tocopy );
        dst += tocopy;
        left -= tocopy;

        // release data page
        release_page( datpg );
    }

    magic = (UINT32) ub  | user_buffer_flag;
    return ub->data + size_overhead;
}

UCHAR *
FileBlockNumber :: get_block( int blockno, UINT32 &magic )
{
    int dummy_size;
    return get_block( blockno, dummy_size, magic );
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

                datpg = get_data_page( pag_num_in_fil );
                pag_num_in_fil++;
                dest = datpg->getrecord( rec_in_pag );
                rec_in_pag = 0;
                tocopy = &datpg->buf[pagesize] - dest;
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

int
FileBlockNumber :: alloc( int bytes )
{
    if ( bytes > 16384 )
    {
        printf( "allocate %d bytes: not allowed\n", bytes );
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
                int c = bm->countbits( bit );

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

    recptr = dat->getrecord( rec_in_pag );
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
        printf( "error, block %d is already marked as free!\n", blockno );
        kill(0,6);
    }
    page * datpg = get_data_page( pag_num_in_fil );

    UCHAR * recptr = datpg->getrecord( rec_in_pag );
    UINT16_t * sig = (UINT16_t*)recptr;
    UINT16_t * szfld = (UINT16_t*)(recptr+2);
    int size = szfld->get();

    if ( sig->get() != block_signature )
    {
        printf( "error in free, block %d does "
                "not have signature!\n", blockno );
        kill(0,6);
    }
    sig->set( 0xeeee );
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

void
FileBlockNumber :: flush( void )
{
    page * p, ** pgs;
    int i, len;

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
        p->clean();
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
}


#ifdef INCLUDE_MAIN

struct testdat {
    bool used;
    int blockno;
    int size;
    UCHAR data[200];
};

#define MAX      20000
#define ROUNDS 1000000
testdat td[ MAX ];

int reps;
int additions;
int deletions;
int modifications;

int
main()
{
    int size, v;
    UINT32 magic;
    UCHAR * buf;
    FILE * lf;
    time_t start, stop, last, now;

    lf = NULL;
//    lf = fopen( "testlog", "w" );

    if ( lf ) setlinebuf( lf );
    int seed;

//    seed = getpid() * time(0);
    seed = -838227822;

    srandom( seed );
    if ( lf ) fprintf( lf, "S %d\n", seed );
    memset( &td, 0, sizeof( td ));
    unlink( "test" );
    FileBlockNumber f( "test", 20, 50, 500 );

    time( &last );
    start = last;

    reps = 0;
    additions = 0;
    deletions = 0;
    modifications = 0;

    int r;
    for ( r = 0; r < ROUNDS; r++ )
    {
        int ind = random() % MAX;
        testdat * t = &td[ind];

        time( &now );
        if ( last != now )
        {
            last = now;
            printf( "reps %d additions %d "
                    "deletions %d modifications %d\n",
                    reps, additions, deletions, modifications );
            reps = 0;
            additions = 0;
            deletions = 0;
            modifications = 0;
        }

//        if ( r == 1793 )
//        {
//            printf( "there!\n" );
//        }

        if ( t->used )
        {
            buf = f.get_block( t->blockno, size, magic );
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

                if ( lf ) fprintf( lf, "M ind %d\n", ind );
                modifications++;
            }
            else if ( v > 33 )
            {
                f.unlock_block( magic, false );
                if ( lf ) fprintf( lf, "V ind %d\n", ind );
            }
            else
            {
                f.unlock_block( magic, false );
                f.free( t->blockno );
                t->used = false;

                if ( lf ) fprintf( lf, "F ind %d\n", ind );
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
            buf = f.get_block( t->blockno, magic );
            memcpy( buf, t->data, size );
            f.unlock_block( magic, true );

            if ( lf ) fprintf( lf, "A ind %d blockno %d size %d\n",
                               ind, t->blockno, t->size );
            additions++;
        }
        reps++;
    }

    time( &stop );

    printf( "completed %d reps in %d seconds, %d reps/second\n",
            ROUNDS, stop - start, ROUNDS / (stop-start) );
}

#endif
