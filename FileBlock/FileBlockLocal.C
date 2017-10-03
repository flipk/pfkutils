
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

    /** \todo */
}

//virtual
FileBlockLocal :: ~FileBlockLocal( void )
{
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
    int i;

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
    m.add( pos, len, /* bogus block id */ 1 );
    pos += len;

    // a free entry for the remainder of the file.
    len = (UINT32) (0xffffffffUL - sizeof(FileBlockHeader));
    m.add( pos, len );

    /* pos==0 has special meaning to store_map, it means
     * it doesn't have to free any previous map. */
    fbh->extent_map_start.set( 0 );
    fbh->extent_map_part_len.set( 0 );

    store_map( &m, bc, &fbh->extent_map_start, &fbh->extent_map_part_len );

    fbh->extent_map_start.set( pos );
    fbh->extent_map_part_len.set( len );

    for (i=0; i < MAX_FILE_INFO_BLOCKS; i++)
        fbh->file_info_block_ids[i].set( 0 );

    bc->release(bcb,true);
}

//virtual
UINT32
FileBlockLocal :: alloc( int size )
{
    /** \todo */

    return 0;
}

//virtual
void
FileBlockLocal :: free( UINT32 id )
{
    /** \todo */
}

//virtual
FileBlock *
FileBlockLocal :: get_block( UINT32 id, bool for_write /*= false*/ )
{
    /** \todo */
    return NULL;
}

//virtual
void
FileBlockLocal :: unlock_block( FileBlock * blk )
{
    /** \todo */
}

//virtual
void
FileBlockLocal :: flush(void)
{
    /** \todo should first sort all of the bucket-lists by file position
     * so that allocations start early in the file. */
}

//virtual
void
FileBlockLocal :: compact(int time_limit)
{
    /** \todo should first sort all of the bucket-lists by file position
     * so that allocations start early in the file. */
}
