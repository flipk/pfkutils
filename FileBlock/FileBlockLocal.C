
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

    BlockCacheT <FileBlockLocalHeader>  fbh(bc);
    fbh.get( FILE_BLOCK_HEADER_POSITION );

    load_map( fbh.data );
    free_map( fbh.data );

    map_present = false;
}

//virtual
FileBlockLocal :: ~FileBlockLocal( void )
{
    flush();
}

//static
bool
FileBlockLocal :: is_valid_file( BlockCache * bc )
{
    BlockCacheT <FileBlockLocalHeader>  fbh(bc);
    bool ret = false;

    if (fbh.get( FILE_BLOCK_HEADER_POSITION ) == false)
        return ret;

    if (memcmp(fbh.data->signature,
               FILE_BLOCK_SIGNATURE, FILE_BLOCK_SIGNATURE_LEN) == 0)
        ret = true;

    return ret;
}

//static
void
FileBlockLocal :: init_file( BlockCache * bc )
{
    BlockCacheT <FileBlockLocalHeader>  fbh(bc);
    int i;

    if (fbh.get( FILE_BLOCK_HEADER_POSITION ) == false)
        return;

    fbh.mark_dirty();

    memcpy(fbh.data->signature,
           FILE_BLOCK_SIGNATURE, FILE_BLOCK_SIGNATURE_LEN);

    // create initial extent map */
    Extents m;
    off_t pos = 0;
    UINT32 len;

    // one entry to cover the file header itself.
    len = sizeof(FileBlockLocalHeader);

    // be sure to round up to the nearest block boundary.
    len = ((((len - 1) >> 5) + 1) << 5);

    m.add( pos, len, /* bogus block id */ 1 );
    pos += len;

    // a free entry for the remainder of the file.  note the final
    // entry of the list always has 7fffffff as the size.
    len = 0x7fffffffUL;
    m.add( pos, len );

    store_map( &m, bc, fbh.data );

    for (i=0; i < MAX_FILE_INFO_BLOCKS; i++)
        fbh.data->file_info_block_ids[i].set( 0 );
}

//virtual
UINT32
FileBlockLocal :: get_data_info_block( char * info_name )
{
    UINT32 id = 0;
    BlockCacheT <FileBlockLocalHeader>  fbh(bc);
    if (fbh.get( FILE_BLOCK_HEADER_POSITION ) == false)
        return 0;

    int i;
    for (i=0; i < MAX_FILE_INFO_BLOCKS; i++)
    {
        FileBlockT <FileInfoBlockId>  fib(this);
        id = fbh.data->file_info_block_ids[i].get();
        if (id == 0)
            continue;
        if (fib.get(id) == false)
            continue;
        if (strncmp(info_name, fib.data->string,
                    sizeof(fib.data->string)) == 0)
            return fib.data->id.get();
    }

    // it wasn't found
    return 0;
}

//virtual
void
FileBlockLocal :: set_data_info_block( UINT32 info_id, char *info_name )
{
    UINT32 id = 0;
    BlockCacheT <FileBlockLocalHeader>  fbh(bc);
    if (fbh.get( FILE_BLOCK_HEADER_POSITION ) == false)
        return;

    int i;
    for (i=0; i < MAX_FILE_INFO_BLOCKS; i++)
    {
        id = fbh.data->file_info_block_ids[i].get();
        if (id == 0)
            break;
    }

    if (id != 0)
    {
        fprintf(stderr, "out of file info blocks!!\n");
        return;
    }

//    fbh.mark_dirty();
    id = alloc( sizeof(FileInfoBlockId) );
    fbh.data->file_info_block_ids[i].set( id );

    FileBlockT <FileInfoBlockId>  fib(this);
    fib.get(id, true);
//    fib.mark_dirty();
    fib.data->id.set( info_id );
    strncpy( fib.data->string, info_name, sizeof(fib.data->string) );
}

//virtual
void
FileBlockLocal :: del_data_info_block( char * info_name )
{
    UINT32 id = 0;
    BlockCacheT <FileBlockLocalHeader>  fbh(bc);
    FileBlockT <FileInfoBlockId>  fib(this);

    if (fbh.get( FILE_BLOCK_HEADER_POSITION ) == false)
        return;

    int i;
    for (i=0; i < MAX_FILE_INFO_BLOCKS; i++)
    {
        id = fbh.data->file_info_block_ids[i].get();
        if (id == 0)
            continue;
        if (fib.get(id) == false)
            continue;
        if (strncmp(info_name, fib.data->string,
                    sizeof(fib.data->string)) == 0)
            break;
    }

    if (i == MAX_FILE_INFO_BLOCKS)
        // it wasn't found
        return;

    if (fib.get(id) == false)
        return;

    free(fib.data->id.get());
    fib.release();
    free(id);

    fbh.data->file_info_block_ids[i].set(0);
    fbh.mark_dirty();
}

//virtual
UINT32
FileBlockLocal :: alloc( int size )
{
    if (map_present)
    {
        BlockCacheT <FileBlockLocalHeader>  fbh(bc);
        fbh.get( FILE_BLOCK_HEADER_POSITION );
        free_map( fbh.data );
        map_present = false;
    }
    Extent * e;
    e = map.alloc( size );
    return e->id;
}

//virtual
void
FileBlockLocal :: free( UINT32 id )
{
    if (map_present)
    {
        BlockCacheT <FileBlockLocalHeader>  fbh(bc);
        fbh.get( FILE_BLOCK_HEADER_POSITION );
        free_map( fbh.data );
        map_present = false;
    }
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
    FileBlockLocalInt * fbl = new FileBlockLocalInt( id, bcb );
    FBLlist.add( fbl );

    return fbl;
}

//virtual
void
FileBlockLocal :: release( FileBlock * blk, bool dirty )
{
    FileBlockLocalInt * fbl = (FileBlockLocalInt *) blk;
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

    {
        BlockCacheT <FileBlockLocalHeader>  fbh(bc);
        fbh.get( FILE_BLOCK_HEADER_POSITION );

        if (map_present)
            free_map( fbh.data );

        store_map( &map, bc, fbh.data );
        fbh.mark_dirty();

        map_present = true;

    }

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
