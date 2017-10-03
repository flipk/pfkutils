
/*
    This file is part of the "pfkutils" tools written by Phil Knaack
    (pknaack1@netscape.net).
    Copyright (C) 2008  Phillip F Knaack

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

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
#include <zlib.h>
#include <fcntl.h>

#define MULTIPLE_FILES 1

file_db :: file_db( char * fname, bool create_it )
{
    struct stat sb;
    int    c1 = 1000;  // ?
    int order =   29;  // ?

    srandom( time(NULL) * getpid() );

    int fnamelen = strlen(fname);
    char fname0[ fnamelen + 6 ]; // ".nodes"
#if MULTIPLE_FILES
    char fname1[ fnamelen + 5 ]; // ".keys"
    char fname2[ fnamelen + 5 ]; // ".data"
    char fname3[ fnamelen + 5 ]; // ".cont"

    sprintf( fname0, "%s.nodes", fname );
    sprintf( fname1, "%s.keys",  fname );
    sprintf( fname2, "%s.data",  fname );
    sprintf( fname3, "%s.cont",  fname );
#else
    strcpy( fname0, fname );
#endif

    // fetch version file
    mytar_sig  sig;
    int  sig_fd, cc;
    sig_fd = open( MYTAR_SIGFILE, O_RDONLY );
    if (sig_fd > 0)
    {
        cc = read( sig_fd, &sig, sizeof(sig) );
        if (cc != sizeof(sig))
        {
            fprintf(stderr,
                    "error: signature file '%s' truncated\n", MYTAR_SIGFILE);
            exit( 1 );
        }
        if (!sig.valid())
        {
            fprintf(stderr,
                    "error: signature file '%s' invalid\n",
                    MYTAR_SIGFILE);
            exit( 1 );
        }
        close(sig_fd);
    }
    else
    {
        // signature file did not exist
        if ( !create_it )
        {
            fprintf(stderr, "warning: signature file '%s' "
                    "not found, creating\n", MYTAR_SIGFILE);
        }
        // create it
        sig.init();
        sig_fd = open( MYTAR_SIGFILE, O_CREAT | O_WRONLY, 0644 );
        if (sig_fd < 0)
        {
            fprintf(stderr, "unable to create signature file: %s\n",
                    strerror(errno));
            exit(1);
        }
        cc = write( sig_fd, &sig, sizeof(sig));
        if (cc != sizeof(sig))
        {
            fprintf(stderr, "unable to write signature file: %s\n",
                    strerror(errno));
            exit(1);
        }
        close( sig_fd );
        chmod( MYTAR_SIGFILE, 0444 );
    }

    if ( create_it )
    {
        if ( stat( fname0, &sb ) == 0 )
        {
            fprintf( stderr, 
                     "file '%s' already exists\n", fname );
            exit( 1 );
        }
    }
    else
    {
        if ( stat( fname0, &sb ) < 0 )
        {
            fprintf( stderr,
                     "unable to stat '%s': %s\n",
                     fname, strerror(errno) );
            exit( 1 );
        }
    }

    fbn_nodes     = new FileBlockNumber( fname0, c1, 32, 32768 );
#if MULTIPLE_FILES
    fbn_keys      = new FileBlockNumber( fname1, c1, 32, 32768 );
    fbn_data      = new FileBlockNumber( fname2, c1, 32, 32768 );
    fbn_contents  = new FileBlockNumber( fname3, c1, 32, 32768 );
#else
    fbn_keys = fbn_data = fbn_contents = fbn_nodes;
#endif
    if ( !fbn_nodes || !fbn_keys || !fbn_data || !fbn_contents )
    {
        fprintf( stderr, 
                 "opening '%s': %s\n", fname, strerror(errno) );
        exit( 1 );
    }

    if ( create_it )
    {
        Btree::new_file( fbn_nodes, order );
    }

    bt = new Btree( fbn_nodes, fbn_keys, fbn_data );
    if ( !bt )
    {
        fprintf( stderr, 
                 "opening btree '%s': %s\n", fname, strerror(errno) );
        exit( 1 );
    }

    Btree::rec * rec;
    if ( create_it )
    {
        Btree::new_file( fbn_nodes, order );
        write_signature( &sig );
    }
    else
    {
        rec = bt->get_rec( (UCHAR*)MYTAR_SIGKEY, sizeof(MYTAR_SIGKEY) );
        if (!rec)
        {
            fprintf(stderr, "NOTE: backup does not have signature; adding\n");
            write_signature( &sig );
        }
        else
        {
            datum_0_data * d0d = (datum_0_data *) rec->data.ptr;
            if (sig.version.get() != d0d->version.get())
            {
                fprintf(stderr, "error:  backup signature does not "
                        "match signature file!\n");
                exit( 1 );
            }
            bt->unlock_rec(rec);
        }
    }

    current_mark = random();
}

void
file_db :: write_signature( mytar_sig * sig )
{
    Btree::rec * rec;
    rec = bt->alloc_rec( sizeof(MYTAR_SIGKEY), sizeof(mytar_sig) );
    strcpy( (char*) rec->key.ptr, MYTAR_SIGKEY );
    datum_0_data * d0d = (datum_0_data *) rec->data.ptr;
    d0d->version = sig->version;
    bt->put_rec( rec );
}

file_db :: ~file_db( void )
{
    delete bt;   // this also deletes fbns
#if MULTIPLE_FILES
    delete fbn_contents;
#endif
}

class file_db_iterate_pi : public btree_printinfo {
public:
    file_db * me;
    FileBlockNumber * fbn_data;
    file_db_iterator * it;
    file_db_iterate_pi( file_db * _me,
                        FileBlockNumber * _fbn_data,
                        file_db_iterator * _it ) :
        btree_printinfo( KEY_REC_PTR /* | DATA_REC_PTR */ ) {
        me = _me; fbn_data = _fbn_data; it = _it;
    }
    // sprintelement should return null if dumptree/dumpnode should stop.
    virtual char * sprint_element( UINT32 noderec,
                                   UINT32 keyrec, void * key, int keylen,
                                   UINT32 datrec, void * dat, int datlen,
                                   bool * datdirty ) {
        struct datum_2_key * d2k = (struct datum_2_key *)key;
        if ( d2k->prefix_i == 'i' )
        {
            ULONG magic;
            int data_size;
            UCHAR * data_block =
                fbn_data->get_block( datrec, &data_size, &magic );
            file_info * fi = me->_get_info_from_block(
                d2k->id.get(), data_block, data_size );
            fbn_data->unlock_block( magic, false );
            if ( fi )
                it->file( fi );
        }
        return (char*) 1;
    }
    // dumpnode will call sprintelementfree when its done
    // actually doing the printing.
    virtual void sprint_element_free( char * s ) {
        // nothing
    }
    // this is the function that actually prints.
    virtual void print( char * format, ... )
    {
        // nothing
    }
};

void
file_db :: iterate( file_db_iterator * it )
{
    file_db_iterate_pi  fdipi( this, fbn_data, it );
    bt->dumptree( &fdipi );
}

file_info *
file_db :: _get_info_from_block( UINT32 id, UCHAR * data_block, int data_len )
{
    file_info * ret;
    datum_2 * d2 = (datum_2 *) data_block;
    ret = new file_info( d2->fname, data_len - sizeof(datum_2) );
    ret->id    = id;
    ret->size  = d2-> size.get();
    ret->mtime = d2->mtime.get();
    ret->uid   = d2->  uid.get();
    ret->gid   = d2->  gid.get();
    ret->mark  = d2-> mark.get();
    ret->mode  = d2-> mode.get();
    ret->rec   = NULL;
    ret->bt    = NULL;
    return ret;
}

file_info *
file_db :: get_info_by_id( UINT32 id )
{
    file_info * ret = NULL;
    Btree::rec * rec;
    datum_2_key  d2k;

    d2k.prefix_i = 'i';
    d2k.id = id;
    rec = bt->get_rec( (UCHAR*)&d2k, sizeof(d2k) );

    if ( rec )
    {
        ret = _get_info_from_block( id, rec->data.ptr, rec->data.len );
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
    datum_1_key * d1k = (datum_1_key *)buf;
    d1k->prefix_n = 'n';
    memcpy( d1k->fname, fname, fname_len );
    Btree::rec * rec = bt->get_rec( (UCHAR*)buf, fname_len+1 );
    delete[] buf;

    if ( rec )
    {
        datum_1 * d1 = (datum_1 *)rec->data.ptr;
        UINT32 id = d1->id.get();
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

    delete inf;
}

UINT32
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
    datum_1_key * d1k = (datum_1_key *) rec->key.ptr;
    datum_1     * d1  = (datum_1 *) rec->data.ptr;

    d1k->prefix_n = 'n';
    memcpy( d1k->fname, inf->fname, fname_len );
    d1->id.set( id );

    bt->put_rec( rec );

    // make the datum_2

    rec = bt->alloc_rec( sizeof(id) + 1,
                         sizeof(datum_2) + fname_len ); // no NUL
    datum_2_key * d2k = (datum_2_key *) rec->key.ptr;
    datum_2     * d2  = (datum_2     *) rec->data.ptr;

    d2k->prefix_i = 'i';
    d2k->  id = id;

    d2-> size = inf->size;
    d2->mtime = inf->mtime;
    d2->  uid = inf->uid;
    d2->  gid = inf->gid;
    d2-> mark = current_mark;
    d2-> mode = inf->mode;

    memcpy( d2->fname, inf->fname, fname_len );
    bt->put_rec( rec );

    delete inf;

    return id;
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

    /*virtual*/ char * sprint_element( UINT32 noderec,
                                       UINT32 keyrec, void * key, int keylen,
                                       UINT32 datrec, void * dat, int datlen,
                                       bool * datdirty )
    {
        datum_2_key * d2k = (datum_2_key *) key;
        if ( d2k->prefix_i == 'i' )
        {
            FileBlockNumber * fbn = bt->get_fbn_data();
            ULONG magic;
            int d2size;
            datum_2 * d2 = (datum_2 *)
                fbn->get_block( datrec, &d2size, &magic );
            if ( d2 )
            {
                if ( d2->mark.get() != current_mark )
                {
                    delete_id * did = new delete_id;
                    did->id = d2k->id.get();
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
    {
        // nothing
    }
};

void
file_db :: delete_old( void )
{
    Btree::rec * d2rec;
    datum_2 * d2;
    UINT32  id;

    file_db_delete_old_pi   pi( bt, current_mark );

    bt->dumptree( &pi );

    delete_id * did;
    while (( did = pi.list.dequeue_head() ))
    {
        id = did->id;
        delete did;

        // locate the datum 2
        datum_2_key  d2k;
        d2k.prefix_i = 'i';
        d2k.id.set( id );
        d2rec = bt->get_rec( (UCHAR*)&d2k, sizeof(d2k) );
        if ( !d2rec )
        {
            fprintf( stderr, "warning, internal error in delete_old\n" );
            continue;
        }
        d2 = (datum_2 *) d2rec->data.ptr;

        // delete the datum 1 using filename located in datum 2
        int fname_len = d2rec->data.len - sizeof(datum_2);
        // only fname_len+1 is used for the key; we add an extra
        // byte so that we can put a nul on the end to make it
        // printable.
        char * d1key = new char[ fname_len + 2 ];
        datum_1_key * d1k = (datum_1_key *) d1key;
        d1k->prefix_n = 'n';
        memcpy( d1k->fname, d2->fname, fname_len );
        d1k->fname[fname_len] = 0;
        printf( "deleting %s\n", d1k->fname );
        (void) bt->delete_rec( (UCHAR*) d1k, fname_len+1 );
        delete[] d1key;

        // delete the datum 2
        bt->delete_rec( d2rec );

        // delete all datum 3's and 4's

        datum_3_key   d3k;
        datum_3     * d3;
        d3k.prefix_d = 'd';
        d3k.id.set( id );
        UINT32 piecenum;
        for ( piecenum = 0; ; )
        {
            d3k.piece_num.set( piecenum );
            Btree::rec * d3rec;
            d3rec = bt->get_rec( (UCHAR*) &d3k, sizeof( d3k ));
            if ( d3rec == NULL )
                break;
            d3 = (datum_3 *) d3rec->data.ptr;
            if ( d3->size.get() > 0 )
                fbn_contents->free( d3->blockno.get() );
            bt->delete_rec( d3rec );
        }
    }
}


void
file_db :: truncate_pieces( UINT32 id, UINT32 num_pieces )
{
    UINT32 piece_num;

    // check if piece# n+1 exists
    //   if so, delete it and repeat check for n+2, etc

    datum_3_key   d3k;
    d3k.prefix_d = 'd';
    d3k.id.set( id );

    for ( piece_num = num_pieces; ; piece_num++ )
    {
        datum_3     * d3;
        Btree::rec  * d3rec;

        d3k.piece_num.set( piece_num );

        d3rec = bt->get_rec( (UCHAR*) &d3k, sizeof(d3k) );
        if ( d3rec == NULL )
            break;
        d3 = (datum_3 *) d3rec->data.ptr;
        if ( d3->size.get() > 0 )
            fbn_contents->free( d3->blockno.get() );
        bt->delete_rec( d3rec );
    }
}

bool
file_db :: update_piece( UINT32 id, UINT32 piece_num,
                         char * buf, int buflen )
{
    MD5_CTX       ctx;
    MD5_DIGEST    dig;
    datum_3_key   d3k;
    datum_3     * d3;
    Btree::rec  * d3rec;
    bool          d3_created = false;
    UINT32        blockno;
    ULONG        block_magic;
    UCHAR       * ptr;

    MD5Init( &ctx );
    MD5Update( &ctx, (unsigned char *) buf, (unsigned int) buflen );
    MD5Final( &dig, &ctx );

    d3k.prefix_d = 'd';
    d3k.id.set( id );
    d3k.piece_num.set( piece_num );

    d3rec = bt->get_rec( (UCHAR*) &d3k, sizeof(d3k ));
    if ( !d3rec )
    {
        d3rec = bt->alloc_rec( sizeof(d3k), sizeof(datum_3) );
        memcpy( d3rec->key.ptr, &d3k, sizeof(d3k) );
        d3 = (datum_3 *) d3rec->data.ptr;
        d3->size.set( 0 );
        d3_created = true;
    }
    else
    {
        d3 = (datum_3 *) d3rec->data.ptr;
        if ( memcmp( &d3->digest, &dig, sizeof(dig) ) == 0 )
        {
            // match! no need to update contents.
            bt->unlock_rec( d3rec );
            return false;
        }
    }

    UCHAR outbuf[ buflen + 64 ];
    uLongf outlen = sizeof(outbuf);

    (void) compress( outbuf, &outlen, (const Bytef*)buf, buflen );

    blockno = d3->blockno.get();
    if ( outlen != d3->size.get() )
    {
        if ( d3->size.get() != 0 )
            fbn_contents->free( blockno );
        blockno = fbn_contents->alloc( outlen );
        d3->size.set( outlen );
        d3->blockno.set( blockno );
    }
    memcpy( &d3->digest, &dig, sizeof(dig) );
    d3rec->data.dirty = true;

    if ( !d3_created )
        bt->unlock_rec( d3rec );
    else
        bt->put_rec( d3rec );

    ptr = fbn_contents->get_block_for_write( blockno, NULL, &block_magic );
    memcpy( ptr, outbuf, outlen );
    fbn_contents->unlock_block( block_magic, true );

    return true;
}

void
file_db :: extract_piece( UINT32 id, UINT32 piece_num,
                          char * buf, int * buflen )
{
    datum_3_key  d3k;

    d3k.prefix_d = 'd';
    d3k.id = id;
    d3k.piece_num = piece_num;

    Btree::rec * d3rec = bt->get_rec( (UCHAR*) &d3k, sizeof(d3k) );
    if ( !d3rec )
    {
        *buflen = 0;
        return;
    }

    datum_3 * d3 = (datum_3 *) d3rec->data.ptr;

    UINT32  db_len = d3->size.get();

    if ( db_len > (UINT32) *buflen )
    {
        fprintf( stderr, "error, block too big!\n" );
        kill(0,6);
    }

    if ( db_len == 0 )
    {
        *buflen = db_len;
    }
    else
    {
        ULONG magic;
        UINT32 blockno;
        UCHAR * ptr;

        blockno = d3->blockno.get();
        ptr = fbn_contents->get_block( blockno, &magic );
        if ( !ptr )
        {
            fprintf( stderr, "internal error in extract_piece!\n" );
            kill(0,6);
        }
        (void) uncompress( (Bytef*)buf, (uLongf*)buflen, ptr, db_len );
        fbn_contents->unlock_block( magic, false );
    }

    bt->unlock_rec( d3rec );
}
