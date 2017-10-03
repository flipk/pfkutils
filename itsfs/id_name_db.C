/*
 * This code is written by Phillip F Knaack. This code is in the
 * public domain. Do anything you want with this code -- compile it,
 * run it, print it out, pass it around, shit on it -- anything you want,
 * except, don't claim you wrote it.  If you modify it, add your name to
 * this comment in the COPYRIGHT file and to this comment in every file
 * you change.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "id_name_db.H"
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include "config.h"

id_name_db :: id_name_db( void )
{
    sprintf( fname, "/tmp/itsfsdb%d.db", random());
    FileBlockNumber * fbn = new FileBlockNumber( fname, CACHESIZE );
    Btree::new_file( fbn, 15 );
    bt = new Btree( fbn );
    chmod( (char*)fname, 0600 );
    fetch_ret = NULL;
    fetch_ret_len = 0;
}

id_name_db :: ~id_name_db( void )
{
    delete bt;
    if ( fetch_ret )
        delete[] fetch_ret;
    unlink( fname );
}

//
//   key: "I" id              data: ftype mount_id path
//   key: "P" mount_id path   data: ftype id
//

void
id_name_db :: add( int mount_id, uchar * path, inode_file_type ftype, int id )
{
    int keylen, datlen;
    int pathlen = strlen( (char*)path ) + 1;
    Btree::rec * rec;

    keylen = 5;
    datlen = pathlen + 5;
    rec = bt->alloc_rec( keylen, datlen );
    rec->key.ptr[0] = 'I';
    memcpy( rec->key.ptr + 1, &id, 4 );
    rec->data.ptr[0] = ftype;
    memcpy( rec->data.ptr + 1, &mount_id, 4 );
    memcpy( rec->data.ptr + 5, path, pathlen );
    bt->put_rec( rec );

    keylen = pathlen + 5;
    datlen = 5;
    rec = bt->alloc_rec( keylen, datlen );
    rec->key.ptr[0] = 'P';
    memcpy( rec->key.ptr + 1, &mount_id, 4 );
    memcpy( rec->key.ptr + 5, path, pathlen );
    rec->data.ptr[0] = ftype;
    memcpy( rec->data.ptr + 1, &id, 4 );
    bt->put_rec( rec );
}

void
id_name_db :: del( int id )
{
    uchar idkey[5];
    Btree::rec * rec;

    idkey[0] = 'I';
    memcpy( idkey + 1, &id, 4 );
    rec = bt->get_rec( idkey, 5 );
    if ( !rec )
        return;

    int datkeylen = rec->data.len;
    uchar * datkey = new uchar[ datkeylen ];
    memcpy( datkey, rec->data.ptr, datkeylen );
    bt->delete_rec( rec );

    datkey[0] = 'P';
    rec = bt->get_rec( datkey, datkeylen );
    if ( rec )
        bt->delete_rec( rec );

    delete[] datkey;
}

uchar *
id_name_db :: fetch( int id, inode_file_type &ftype )
{
    uchar idkey[5];
    Btree::rec * rec;

    idkey[0] = 'I';
    memcpy( idkey + 1, &id, 4 );
    rec = bt->get_rec( idkey, 5 );
    if ( !rec )
        return NULL;

    if ( fetch_ret_len < rec->data.len )
    {
        if ( fetch_ret )
            delete[] fetch_ret;
        fetch_ret = new uchar[ rec->data.len ];
        fetch_ret_len = rec->data.len;
    }

    ftype = (inode_file_type) rec->data.ptr[0];
    memcpy( fetch_ret, rec->data.ptr + 5, rec->data.len - 5 );
    bt->unlock_rec( rec );
    return fetch_ret;
}

int
id_name_db :: fetch( int mount_id, uchar * path, inode_file_type &ftype )
{
    uchar * pkey;
    int pathlen = strlen( (char*)path ) + 1;
    Btree::rec * rec;
    int ret = -1;

    pkey = new uchar[ pathlen + 5 ];
    pkey[0] = 'P';
    memcpy( pkey + 1, &mount_id, 4 );
    memcpy( pkey + 5, path, pathlen );
    rec = bt->get_rec( pkey, pathlen + 5 );
    delete[] pkey;

    if ( !rec )
        return -1;

    ftype = (inode_file_type) rec->data.ptr[0];
    memcpy( &ret, rec->data.ptr + 1, 4 );

    bt->unlock_rec( rec );
    return ret;
}



struct btcollect {
    uchar cmpkey[ 5 ];
    int numalloc;
    int numused;
    int * ids;
    btcollect( void )
        {
            numalloc = 100;
            numused = 0;
            ids = (int*) malloc( sizeof( int ) * numalloc );
        }
    ~btcollect( void )
        {
            free( ids );
        }
    void add( int id )
        {
            if ( numalloc == numused )
            {
                numalloc *= 2;
                ids = (int *) realloc( ids, 
                                       sizeof( int ) * numalloc );
            }
            ids[numused++] = id;
        }
};

//static
char *
id_name_db :: btdump_sprint( void * arg, int noderec,
                             int keyrec, void * key, int keylen,
                             int datrec, void * dat, int datlen )
{
    btcollect * btc = (btcollect *)arg;
    if ( memcmp( btc->cmpkey, key, 5 ) == 0 )
    {
        int id;
        memcpy( &id, (uchar*)dat + 1, 4 );
        btc->add( id );
    }

    // return non-null so dumptree doesn't stop here.
    return (char*) 1;
}

//static
void
id_name_db :: btdump_sprintfree( void * arg, char * s )
{
    // does nothing!
}

//static
void
id_name_db :: btdump_print( void * arg, char * format, ... )
{
    // does nothing!
}

void
id_name_db :: purge_mount( int mount_id )
{
    Btree::printinfo pi;
    uchar cmpstr[ 5 ];
    btcollect btc;

    btc.cmpkey[0] = 'P';
    memcpy( btc.cmpkey + 1, &mount_id, 4 );

    pi.spr   = &btdump_sprint;
    pi.sprf  = &btdump_sprintfree;
    pi.pr    = &btdump_print;
    pi.arg   = &btc;
    pi.debug = false;

    bt->dumptree( &pi );

    for ( int i = 0; i < btc.numused; i++ )
    {
        del( btc.ids[i] );
    }

}
