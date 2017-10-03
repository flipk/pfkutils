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
#include "lognew.H"
#include "config.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

id_name_db :: id_name_db( void )
{
    sprintf( fname, "/tmp/itsfsdb%d.db", random());
    FileBlockNumber * fbn = LOGNEW FileBlockNumber( fname, CACHESIZE );
    Btree::new_file( fbn, 15 );
    bt = LOGNEW Btree( fbn );
    chmod( (char*)fname, 0600 );
    purge_active = false;
    purging_id = 0;
}

id_name_db :: ~id_name_db( void )
{
    delete bt;
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
    uchar * datkey = LOGNEW uchar[ datkeylen ];
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
    uchar * ret;
    Btree::rec * rec;

    idkey[0] = 'I';
    memcpy( idkey + 1, &id, 4 );
    rec = bt->get_rec( idkey, 5 );
    if ( !rec )
        return NULL;

    ret = LOGNEW uchar[ rec->data.len ];

    ftype = (inode_file_type) rec->data.ptr[0];
    memcpy( ret, rec->data.ptr + 5, rec->data.len - 5 );
    bt->unlock_rec( rec );
    return ret;
}

int
id_name_db :: fetch( int mount_id, uchar * path, inode_file_type &ftype )
{
    uchar * pkey;
    int pathlen = strlen( (char*)path ) + 1;
    Btree::rec * rec;
    int ret = -1;

    pkey = LOGNEW uchar[ pathlen + 5 ];
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
    int deleted_count;
    int numalloc;
    int numused;
    int * ids;
    bool finished;
    btcollect( void )
        {
            numalloc = 100;
            numused = 0;
            ids = (int*) MALLOC( sizeof( int ) * numalloc );
        }
    ~btcollect( void )
        {
            free( ids );
        }
    void add( int id )
        {
            if ( numalloc == numused )
            {
                int newnumalloc = numalloc * 2;
                int * newids = (int*) MALLOC( sizeof( int ) * newnumalloc );
                memcpy( newids, ids, sizeof( int ) * numalloc );
                free( ids );
                ids = newids;
                numalloc = newnumalloc;
            }
            ids[numused++] = id;
        }
};

//static
char *
id_name_db :: btdump_sprint( void * arg, int noderec,
                             int keyrec, void * key, int keylen,
                             int datrec, void * dat, int datlen,
                             bool *datdirty )
{
    btcollect * btc = (btcollect *)arg;

    if ( memcmp( btc->cmpkey, key, 5 ) == 0 )
    {
        int id;
        memcpy( &id, (uchar*)dat + 1, 4 );
        btc->add( id );
        if ( btc->deleted_count == 0 )
            return 0;
        btc->deleted_count--;
        btc->finished = false;
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
    if ( purge_active )
    {
        // if a backgrounded period purge is already active,
        // don't attempt to background this one, just do it now.
        // the backgrounded one will continue when this one is done.
        // (note, it would actually be better to background this one
        //  too but it makes the implementation more complicated)
        _periodic_purge( true, mount_id );
        return;
    }
    purging_id = mount_id;
    purge_active = true;
}

void
id_name_db :: _periodic_purge( bool all, int id )
{
    uchar cmpstr[ 5 ];
    btcollect btc;
    Btree::printinfo pi = {
        btdump_sprint, btdump_sprintfree, btdump_print, &btc, false
    };

    btc.cmpkey[0] = 'P';
    memcpy( btc.cmpkey + 1, &id, 4 );

    btc.finished = true;
    if ( all )
        btc.deleted_count = -1;
    else
        btc.deleted_count = 250;

    bt->dumptree( &pi );

    for ( int i = 0; i < btc.numused; i++ )
    {
        del( btc.ids[i] );
    }

    if ( btc.finished )
        purge_active = false;
}
#include <stdarg.h>

//static
char *
id_name_db :: btdump_real_sprint( void * arg, int noderec,
                                  int keyrec, void * _key, int keylen,
                                  int datrec, void * _dat, int datlen,
                                  bool *datdirty )
{
    int i;
    unsigned char * key = (unsigned char*) _key;
    unsigned char * dat = (unsigned char*) _dat;
    char * out;
    char * outp;

    out = new char[ keylen * 3 + datlen * 3 + 50 ];
    outp = out;

    outp += sprintf( outp, "key:" );
    for ( i = 0; i < keylen; i++ )
        outp += sprintf( outp, " %02x", key[i] );
    outp += sprintf( outp, "\ndat: " );
    for ( i = 0; i < datlen; i++ )
        outp += sprintf( outp, " %02x", dat[i] );
    outp += sprintf( outp, "\n" );

    // return non-null so dumptree doesn't stop here.
    return out;
}

//static
void
id_name_db :: btdump_real_sprintfree( void * arg, char * s )
{
    delete[] s;
}

//static
void
id_name_db :: btdump_real_print( void * arg, char * format, ... )
{
    va_list ap;
    va_start( ap, format );
    vfprintf( (FILE*)arg, format, ap );
    va_end( ap );
}

void
id_name_db :: dump_btree( void )
{
    Btree::printinfo pi;
    FILE * f;

#define DUMPFILE "BTREEDUMP"

    f = fopen( DUMPFILE, "w" );

    pi.spr   = &btdump_real_sprint;
    pi.sprf  = &btdump_real_sprintfree;
    pi.pr    = &btdump_real_print;
    pi.arg   = (void*)f;
    pi.debug = false;

    bt->dumptree( &pi );

    fclose( f );
    chown( DUMPFILE, 1000, 1000 );
    chmod( DUMPFILE, 0666 );

#undef DUMPFILE

}
