
/** \file FileBlockLocal.C
 * \brief implements FileBlockLocal class
 * \author Phillip F Knaack
 *
 * This file implements the FileBlockLocal class.
 */

#include "FileBlockLocal.H"
#include "FileBlockLocal_internal.H"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

FileBlockLocal :: FileBlockLocal( BlockCache * _bc )
{
    bc = _bc;

    if ( !is_valid_file(bc) )
    {
        fprintf(stderr, "not a valid FileBlockLocal file!\n");
        exit(1);
    }

    FileBlockHeader * fbh;
    BlockCacheBlock * bcb;

    bcb = bc->get( 0, sizeof(FileBlockHeader) );
    if (!bcb)
    {
        fprintf(stderr, "bc->get 0 failed!\n");
        exit(1);
    }

    fbh = (FileBlockHeader *)bcb->get_ptr();

    load_map( fbh->piece_map_start.get(), 
              fbh->piece_map_len.get() );

    bc->release( bcb, false );
}

//virtual
FileBlockLocal :: ~FileBlockLocal( void )
{
    flush();
    /** \todo */
}

//static
bool
FileBlockLocal :: is_valid_file( BlockCache * bc )
{
    FileBlockHeader * fbh;
    BlockCacheBlock * bcb;
    bool ret = false;

    bcb = bc->get( 0, sizeof(FileBlockHeader) );
    if (!bcb)
        return ret;

    fbh = (FileBlockHeader *)bcb->get_ptr();
    if (memcmp(fbh->signature,
               FILE_BLOCK_SIGNATURE, FILE_BLOCK_SIGNATURE_LEN) == 0)
        ret = true;

    bc->release(bcb,false);
    return ret;
}

//static
void
FileBlockLocal :: init_file( BlockCache * bc )
{
    FileBlockHeader * fbh;
    BlockCacheBlock * bcb;
//    int i;

    bcb = bc->get( 0, sizeof(FileBlockHeader) );
    if (!bcb)
        return;

    fbh = (FileBlockHeader *)bcb->get_ptr();
    memcpy(fbh->signature,
           FILE_BLOCK_SIGNATURE, FILE_BLOCK_SIGNATURE_LEN);

    // create initial extent map */
    Extents m;
    off_t pos = 0;
    UINT32 len;

    // one entry to cover the file header itself.
    len = sizeof(FileBlockHeader);

    // be sure to round up to the nearest block boundary.
    len = ((((len - 1) >> 5) + 1) << 5);

    m.add( pos, len, /* bogus block id */ 1 );
    pos += len;

    // a free entry for the remainder of the file.  note the final
    // entry of the list always has 7fffffff as the size because it
    // always represents 'from here to infinity'.
    len = 0x7fffffffUL;
    m.add( pos, len );

    store_map( &m, bc, &fbh->piece_map_start, &fbh->piece_map_len );

//    for (i=0; i < MAX_FILE_INFO_BLOCKS; i++)
//        fbh->file_info_block_ids[i].set( 0 );

    bc->release(bcb,true);
}

//virtual
UINT32
FileBlockLocal :: alloc( int size )
{
    Extent * e;
    e = map.alloc( size );
    return e->id;
}

//virtual
void
FileBlockLocal :: free( UINT32 id )
{
    map.free_id( id );
}

//virtual
FileBlock *
FileBlockLocal :: get( UINT32 id, bool for_write /*= false*/ )
{
    Extent * e;

    e = map.find_id( id );
    if (!e)
        return NULL;

    BlockCacheBlock * bcb = bc->get( e->offset, e->size, for_write );
    FBL * fbl = new FBL( id, bcb );
    FBLlist.add( fbl );

    return fbl;
}

//virtual
void
FileBlockLocal :: release( FileBlock * blk, bool dirty )
{
    FBL * fbl = (FBL *) blk;
    if (dirty)
        fbl->mark_dirty();
    FBLlist.remove( fbl );
    bc->release( fbl->get_bcb() );
    delete fbl;
}

//virtual
void
FileBlockLocal :: flush(void)
{
    // should first sort all of the bucket-lists by file position
    // so that allocations start early in the file?

    FileBlockHeader * fbh;
    BlockCacheBlock * bcb;

    bcb = bc->get( 0, sizeof(FileBlockHeader) );
    if (!bcb)
        return;

    fbh = (FileBlockHeader *)bcb->get_ptr();

    free_map( fbh->piece_map_start.get(), 
              fbh->piece_map_len.get() );

    store_map( &map, bc, &fbh->piece_map_start, &fbh->piece_map_len );

    bc->release(bcb,true);

    bc->flush();
}

//virtual
void
FileBlockLocal :: compact(int time_limit)
{
    // should first sort all of the bucket-lists by file position
    // so that allocations start early in the file?
    // free_map
    // store_map
}
