
#include "btree.H"
#define FILE_DB_INTERNAL
#include "file_db.H"

#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

file_db :: file_db( char * fname, bool create_it )
{
    struct stat sb;
    int    h1 = 1000;  // ?
    int    h2 = 1000;  // ?
    int    c1 = 1000;  // ?
    int order =   29;  // ?

    if ( create_it )
    {
        if ( stat( fname, &sb ) == 0 )
        {
            fprintf( stderr, 
                     "file '%s' already exists\n", fname );
            exit( 1 );
        }
    }
    else
    {
        if ( stat( fname, &sb ) < 0 )
        {
            fprintf( stderr,
                     "unable to stat '%s': %s\n",
                     fname, strerror(errno) );
            exit( 1 );
        }
    }

    fbn = new FileBlockNumber( fname, h1, h2, c1 );
    if ( !fbn )
    {
        fprintf( stderr, 
                 "opening '%s': %s\n", fname, strerror(errno) );
        exit( 1 );
    }

    if ( create_it )
        Btree::new_file( fbn, order );

    bt = new Btree( fbn );
    if ( !bt )
    {
        fprintf( stderr, 
                 "opening btree '%s': %s\n", fname, strerror(errno) );
        exit( 1 );
    }

    srandom( time(NULL) * getpid() );
    current_mark = random();
}

file_db :: ~file_db( void )
{
    delete bt;   // this also deletes fbn
}

//static
void
file_db :: calc_blocks( UINT64 file_size,
                        UINT32 * num_blocks,
                        UINT32 * last_block_size )
{
    UINT32 quotient, remainder;

    quotient  = file_size / BLOCK_SIZE;
    remainder = file_size % BLOCK_SIZE;

    if ( remainder != 0 )
        quotient++;

    *num_blocks      = quotient;
    *last_block_size = remainder;
}

file_info *
file_db :: get_info_by_id( UINT32 id )
{
    file_info * ret = NULL;
    Btree::rec * rec;
    char buf[ sizeof(id) + 1 ];

    buf[0] = 'i';
    memcpy( buf+1, &id, sizeof(id) );
    rec = bt->get_rec( (UCHAR*)buf, sizeof(buf) );

    if ( rec )
    {
        datum_2 * d2 = (datum_2 *) rec->data.ptr;

        ret = new file_info( d2->fname,
                             rec->data.len - sizeof(datum_2) );
        ret->id    = id;
        ret->size  = d2-> size.get();
        ret->mtime = d2->mtime.get();
        ret->uid   = d2->  uid.get();
        ret->gid   = d2->  gid.get();
        ret->mark  = d2-> mark.get();
        ret->mode  = d2-> mode.get();

        ret->rec = rec;
        ret->bt  = bt;
    }

    return ret;
}

file_info *
file_db :: get_info_by_fname( char * fname )
{
    int fname_len = strlen(fname); // not counting nul

    // not counting nul but counting type prefix:
    char * buf = new char[ fname_len + 1 ];
    buf[0] = 'n';
    memcpy( buf+1, fname, fname_len );
    Btree::rec * rec = bt->get_rec( (UCHAR*)buf, fname_len+1 );
    delete[] buf;

    if ( rec )
    {
        UINT32 id;
        memcpy( &id, rec->data.ptr, sizeof(id) );
        bt->unlock_rec( rec );
        return get_info_by_id( id );
    }
    // else
    return NULL;
}

void
file_db :: update_info( file_info * inf )
{
    Btree::rec * rec = inf->rec;
    inf->rec = NULL;

    if ( !rec )
    {
        fprintf( stderr, "internal error in update_info!\n" );
        kill(0,6);
    }

    datum_2 * d2 = (datum_2 *) rec->data.ptr;

    d2->size  = inf->size;
    d2->mtime = inf->mtime;
    d2->mtime = inf->mtime;
    d2->uid   = inf->uid;
    d2->gid   = inf->gid;
    d2->mark  = current_mark;
    d2->mode  = inf->mode;

    rec->data.dirty = true;
    bt->unlock_rec( rec );

    sync_blocklist( inf );
    delete inf;
}

void
file_db :: add_info( file_info * inf )
{
    int fname_len = strlen(inf->fname);  // not counting trailing NUL
    Btree::rec * rec;
    UINT32 id;
    file_info * tmpfi;

    // locate an unused id #
    while ( 1 )
    {
        id = random();
        tmpfi = get_info_by_id( id );
        if ( !tmpfi )
            break;
        delete tmpfi;
    }

    // make the datum_1

    rec = bt->alloc_rec( fname_len+1, // count 'n' but not NUL
                         sizeof(id) );
    ((char*)rec->key.ptr)[0] = 'n';
    memcpy( rec->key.ptr+1, inf->fname, fname_len );
    *(UINT32*)rec->data.ptr = id;
    bt->put_rec( rec );

    // make the datum_2

    rec = bt->alloc_rec( sizeof(id) + 1,
                         sizeof(datum_2) + fname_len ); // no NUL
    ((char*)rec->key.ptr)[0] = 'i';
    memcpy( rec->key.ptr+1, &id, sizeof(id) );
    datum_2 * d2 = (datum_2 *) rec->data.ptr;

    d2-> size = inf->size;
    d2->mtime = inf->mtime;
    d2->  uid = inf->uid;
    d2->  gid = inf->gid;
    d2-> mark = current_mark;
    d2-> mode = inf->mode;

    memcpy( d2->fname, inf->fname, fname_len );
    bt->put_rec( rec );

    sync_blocklist( inf );
}

void
file_db :: sync_blocklist( file_info * inf )
{
    char key[ sizeof(UINT32)*2 + 1 ];
    UINT32 num_blocks, last_block_size;
    UINT32 block_num;

    memcpy( key+1, &inf->id, sizeof(UINT32) );

    calc_blocks( inf->size, &num_blocks, &last_block_size );

    // check if block# n+1 exists
    //   if so, delete it and repeat check for n+2, etc

    for ( block_num = num_blocks; ; block_num++ )
    {
        memcpy( key+5, &block_num, sizeof(UINT32) );
        key[0] = 'd';
        if ( bt->delete_rec( (UCHAR*)key,
                             sizeof(key) ) == Btree::DELETE_KEY_NOT_FOUND )
            break;
        key[0] = 'D';
        (void) bt->delete_rec( (UCHAR*)key, sizeof(key) );
    }
}

struct delete_id {
    LListLinks <delete_id> links[1];
    UINT32  id;
};
typedef LList <delete_id,0> delete_id_list;

class file_db_delete_old_pi : public btree_printinfo {
public:
    delete_id_list  list;
    Btree * bt;
    UINT32 current_mark;
//
    file_db_delete_old_pi( Btree * _bt, UINT32 _current_mark ) :
        btree_printinfo( KEY_REC_PTR )
    {
        bt = _bt;
        current_mark = _current_mark;
    }
    /*virtual*/ ~file_db_delete_old_pi( void ) { /* nothing */ }

    /*virtual*/ char * sprint_element( int noderec,
                                       int keyrec, void * key, int keylen,
                                       int datrec, void * dat, int datlen,
                                       bool * datdirty )
    {
        if ( ((char*)key)[0] == 'i' )
        {
            FileBlockNumber * fbn = bt->get_fbn();
            UINT32 magic;
            datum_2 * d2 = (datum_2 *) fbn->get_block( datrec, magic );
            if ( d2 )
            {
                if ( d2->mark.get() != current_mark )
                {
                    delete_id * did = new delete_id;
                    memcpy( &did->id, ((char*)key)+1, sizeof(UINT32) );
                    printf( "adding id %u to the delete list\n",
                            did->id );
                    list.add( did );
                }
                fbn->unlock_block( magic, false );
            }
        }

        return (char*) 1;
    }
    /*virtual*/ void sprint_element_free( char * s ) {
        // nothing
    }
    /*virtual*/ void print( char * format, ... )
        __attribute__ ((format( printf, 2, 3 ))) {
        // nothing
    }
};

void
file_db :: delete_old( void )
{
    Btree::rec * d2rec;
    char key[9];
    datum_2 * d2;
    UINT32  id, blocknum;

    file_db_delete_old_pi   pi( bt, current_mark );

    bt->dumptree( &pi );

    delete_id * did;
    while ( did = pi.list.dequeue_head() )
    {
        id = did->id;
        delete did;

        // locate the datum 2
        key[0] = 'i';
        memcpy( key+1, &id, sizeof(UINT32) );
        d2rec = bt->get_rec( (UCHAR*)key, 5 );
        if ( !d2rec )
        {
            fprintf( stderr, "warning, internal error in delete_old\n" );
            continue;
        }
        d2 = (datum_2 *) d2rec->data.ptr;

        // delete the datum 1 using filename located in datum 2
        int fname_len = d2rec->data.len - sizeof(datum_2);
        char * d2key = new char[ fname_len + 1 ];
        d2key[0] = 'n';
        memcpy( d2key+1, d2->fname, fname_len );
        (void) bt->delete_rec( (UCHAR*) d2key, fname_len+1 );
        delete[] d2key;

        // delete the datum 2
        bt->delete_rec( d2rec );

        // delete all datum 3's and 4's using 'size' field from datum 2
        for ( blocknum = 0; ; )
        {
            memcpy( key+5, &blocknum, sizeof(UINT32) );
            key[0] = 'd';
            if ( bt->delete_rec( (UCHAR*) key,
                                 9 ) == Btree::DELETE_KEY_NOT_FOUND )
                break;
            key[0] = 'D';
            (void) bt->delete_rec( (UCHAR*) key, 9 );
        }
    }
}

void
file_db :: update_block( file_info * inf, UINT32 block_num,
                         char * buf, int buflen )
{
    MD5_CTX       ctx;
    MD5_DIGEST    dig;
    char          key[9];
    datum_3     * d3;
    Btree::rec  * d3rec;
    Btree::rec  * d4rec;
    bool          d3_created = false;

    MD5Init( &ctx );
    MD5Update( &ctx, (unsigned char *) buf, (unsigned int) buflen );
    MD5Final( &dig, &ctx );

    memcpy( key+1, &inf->id, sizeof(UINT32) );
    memcpy( key+5, &block_num, sizeof(UINT32) );

    key[0] = 'd';
    d3rec = bt->get_rec( (UCHAR*) key, sizeof(key) );
    if ( !d3rec )
    {
        d3rec = bt->alloc_rec( sizeof(key), sizeof(datum_3) );
        memcpy( d3rec->key.ptr, key, sizeof(key) );
        d3 = (datum_3 *) d3rec->data.ptr;
        d3_created = true;
    }
    else
    {
        d3 = (datum_3 *) d3rec->data.ptr;
        if ( memcmp( &d3->digest, &dig, sizeof(dig) ) == 0 )
        {
            // match! no need to update contents.
            bt->unlock_rec( d3rec );
            return;
        }
    }

    d3->bsize = buflen;
    memcpy( &d3->digest, &dig, sizeof(dig) );
    d3rec->data.dirty = true;
    if ( !d3_created )
        bt->unlock_rec( d3rec );
    else
        bt->put_rec( d3rec );

    key[0] = 'D';
    (void) bt->delete_rec( (UCHAR*) key, sizeof(key) );
    d4rec = bt->alloc_rec( sizeof(key), buflen );
    memcpy( d4rec->key.ptr, key, sizeof(key) );
    memcpy( d4rec->data.ptr, buf, buflen );
    bt->put_rec( d4rec );
}

void
file_db :: extract_block( file_info * inf, UINT32 block_num,
                          char * buf, int * buflen )
{
    
}
