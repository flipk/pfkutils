
#include "recno.H"
#include <time.h>

FileRecordNumber :: FileRecordNumber( char * filename,
                                      int recsize, int _hashsize,
                                      int _max_cache_size )
    throw ( constructor_failed )
{
    record_length = recsize;
    fd = fopen( filename, "r+b" );
    if ( fd == NULL )
    {
        fd = fopen( filename, "w+b" );
        if ( fd == NULL )
            throw constructor_failed();
    }
    hashsize = _hashsize;
    numhash = 0;
    datahash = new frn_record*[ hashsize ];
    for ( int i = 0; i < hashsize; i++ )
        datahash[i] = NULL;
    fseek( fd, 0, SEEK_END );
    fseek( fd, 0, SEEK_SET );
    cache_size = 0;
    max_cache_size = _max_cache_size;
    lru_head = lru_tail = NULL;
    flushes = 0;
}

FileRecordNumber :: ~FileRecordNumber( void )
{
    int i;
    frn_record * x;

    // first unlock all records
    for ( i = 0; i < hashsize; i++ )
        for ( x = datahash[i]; x; x = x->next )
        {
            if ( x->locked )
            {
                printf( "frn::~frn : block left locked! forcing dirty\n" );
                x->dirty = true;
                x->clean = -1;
                x->locked = false;
            }
        }

    // then flush all records; since they're all unlocked
    // now, they're guaranteed to all get flushed.
    flush();

    // now delete them all. the second frn_record variable
    // is because we can't follow a next-ptr in a deleted record
    for ( i = 0; i < hashsize; i++ )
    {
        frn_record *y;
        for ( x = datahash[i]; x; x = y )
        {
            y = x->next;
            delete x;
        }
    }

    delete[] datahash;
    fclose( fd );
}

void
FileRecordNumber :: dumpcache( void )
{
    frn_record * x;
    int i;
    printf( "hash:\n" );
    for ( i = 0; i < hashsize; i++ )
    {
        printf( "hash %d: ", i );
        for ( x = datahash[i]; x; x = x->next )
            printf( "%d[%c%c] ",
                    x->recno,
                    x->dirty ? 'd' : ' ',
                    x->locked ? 'l' : ' ' );
        printf( "\n" );
    }

    printf( "lru:\n" );
    for ( x = lru_head; x; x = x->lru_next )
        printf( "%d ", x->recno );
    printf( "\n" );
}

int
FileRecordNumber :: __get_record( frn_record *x )
{
    // error check.
    if ( fseek( fd, record_length * x->recno, SEEK_SET ) != 0 )
        return -1;
    if ( fread( x->dat, record_length, 1, fd ) == 0 )
        // fake it.
        memset( x->dat, 0, record_length );
    return 0;
}

FileRecordNumber :: frn_record *
FileRecordNumber :: _get_record( int recno )
{
    frn_record * x;
    x = find_hash( recno );
    if ( x != NULL )
    {
        delete_lru( x );
        return x;
    }

    x = new( record_length) frn_record( recno );
    if ( __get_record( x ) != 0 )
    {
        delete x;
        return NULL;
    }

    add_hash( x );

    return x;
}

UCHAR *
FileRecordNumber :: get_record( int recno, UINT32& magic )
{
    frn_record * x;

    x = _get_record( recno );
    magic = (UINT32)x;
    return x->dat;
}

int
FileRecordNumber :: __put_record( frn_record *x )
{
    // error check.
    if ( fseek( fd, record_length * x->recno, SEEK_SET ) != 0 )
        return -1;
    if ( fwrite( x->dat, record_length, 1, fd ) != 1 )
        return -1;
    return 0;
}

UCHAR *
FileRecordNumber ::  get_empty_record( int recno, UINT32& magic )
{
    frn_record * x;

    x = find_hash( recno );

    if ( x != NULL )
    {
        delete_lru( x );
    }
    else
    {
        x = new( record_length ) frn_record( recno );
        add_hash( x );
    }

    x->dirty = 1;
    x->clean = -1;

    magic = (UINT32)x;
    return x->dat;
}

void
FileRecordNumber :: unlock_record( UINT32 magic, bool dirty )
{
    frn_record * x = (frn_record*)magic;
    if ( dirty )
    {
        x->dirty = 1;
        x->clean = -1;
    }
    // don't clear dirty flag if dirty==false,
    // because someone else may have set it.
    add_lru( x );
}

int
FileRecordNumber :: flush( void )
{
    int now = time( NULL );
    int r = 0;
    frn_record * x, * xn;
    for ( int i = 0; i < hashsize; i++ )
        for ( x = datahash[i]; x; x = xn )
        {
            xn = x->next;
            if ( x->dirty )
            {
                r++;
                __put_record( x );
                if ( !x->locked )
                {
                    x->dirty = 0;
                    x->clean = time( NULL );
                }
            }
            else
            {
                if ( x->locked == 0  &&  x->clean != -1  && 
                     ( now - x->clean ) > 30 )
                {
                    delete_lru( x );
                    del_hash( x );
                    delete x;
                }
            }
        }
    fflush( fd );
    r += flushes;
    flushes = 0;
    return r;
}

//
// the lru is a linked list, starting at lru_head
// and ending at lru_tail. lru_next pointers point
// towards tail, lru_prev pointers point towards
// head.
//

FileRecordNumber :: frn_record *
FileRecordNumber :: get_lru( void )
{
    frn_record * ret = lru_head;
    delete_lru( ret );
    return ret;
}

void
FileRecordNumber :: add_lru( frn_record * x )
{
    if ( x->locked == 0 )
    {
        printf( "FileRecordNumber: unlocking a record "
                "already unlocked %s:%d\n",
                __FILE__, __LINE__ );
        return;
    }

    if ( cache_size == max_cache_size )
    {
        bool needsflush = false;
        frn_record * y = get_lru();
        if ( y->dirty )
        {
            needsflush = true;
            flushes++;
            __put_record( y );
        }
        del_hash( y );
        delete y;
        if ( needsflush )
            flushes = flush();
    }

    cache_size++;
    x->locked = 0;

    if ( lru_tail == NULL )
    {
        lru_head = lru_tail = x;
        x->lru_next = x->lru_prev = NULL;
    }
    else
    {
        x->lru_prev = lru_tail;
        x->lru_prev->lru_next = x;
        x->lru_next = NULL;
        lru_tail = x;
    }
}

void
FileRecordNumber :: delete_lru( frn_record * x )
{
    if ( x->locked == 1 )
        return;

    if ( x->lru_next )
        x->lru_next->lru_prev = x->lru_prev;
    else
        lru_tail = x->lru_prev;

    if ( x->lru_prev )
        x->lru_prev->lru_next = x->lru_next;
    else
        lru_head = x->lru_next;

    x->lru_next = NULL;
    x->lru_prev = NULL;
    cache_size--;
    x->locked = 1;
}

void
FileRecordNumber :: add_hash( frn_record * x )
{
    h = hash( x->recno );
    x->next = datahash[ h ];
    if ( x->next )
        x->next->prev = x;
    x->prev = NULL;
    datahash[ h ] = x;
    numhash++;
}

void
FileRecordNumber :: del_hash( frn_record * x )
{
    h = hash( x->recno );

    if ( x->next )
        x->next->prev = x->prev;

    if ( x->prev )
        x->prev->next = x->next;
    else
        datahash[ h ] = x->next;

    numhash--;
}

FileRecordNumber :: frn_record *
FileRecordNumber :: find_hash( int recno )
{
    h = hash( recno );
    frn_record * ret;
    for ( ret = datahash[h]; ret; ret = ret->next )
        if ( ret->recno == recno )
            return ret;
    return NULL;
}
