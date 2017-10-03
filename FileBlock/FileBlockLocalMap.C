
/** \file FileBlockLocalMap.C
 * \brief implements FileBlockLocal::store_map function
 * \author Phillip F Knaack
 *
 * This file implements the FileBlockLocal::store_map function,
 * which is surprisingly complicated.
 */

#include "FileBlockLocal.H"
#include "FileBlockLocal_internal.H"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

void
FileBlockLocal :: load_map( UINT64 pos, UINT32 len )
{
    PieceMapEntry * pme;
    BlockCacheBlock * piecemap_block;

    piecemap_block = bc->get( pos, len );

    pme = (PieceMapEntry *) piecemap_block->get_ptr();

    UINT64  current_pos = 0;
    UINT64  piece_offset;
    UINT32  piece_len;
    BlockCacheBlock * piece_block = NULL;
    UINT32_t * entries = NULL;
    UINT32 j = 0;

#define GET_VALUE(val)                                              \
    {                                                               \
        if (entries == NULL)                                        \
        {                                                           \
            piece_offset = pme->offset.get();                       \
            if (piece_offset == 0)                                  \
                break;                                              \
            piece_len = pme->len.get();                             \
            pme++;                                                  \
            printf("piece at 0x%llx is 0x%x bytes long (%d entries)\n", \
                   piece_offset, piece_len, piece_len/4);           \
            piece_block = bc->get( piece_offset, piece_len );       \
            entries = (UINT32_t *) piece_block->get_ptr();          \
            j = 0;                                                  \
        }                                                           \
        (val) = entries[j].get();                                   \
        if (++j == (piece_len / sizeof(UINT32_t)))                  \
        {                                                           \
            bc->release( piece_block, false );                      \
            piece_block = NULL;                                     \
            entries = NULL;                                         \
        }                                                           \
    }

    while (1)
    {
        UINT32 val;
        UINT32 size;

        GET_VALUE(val);

        size = val & 0x7FFFFFFF;

        if (size == 0)
            break;

        if (val & 0x80000000)
        {
            // add a used entry, first extract id
            GET_VALUE(val);
            printf("adding used entry for offset %lld size %d id 0x%x\n",
                   current_pos, size, val );
            map.add( current_pos, size, val );
        }
        else
        {
            // add a free entry
            printf("adding free entry for offset %lld size %d\n",
                   current_pos, size);
            map.add( current_pos, size );
        }
        current_pos += size;
    }

#undef GET_VALUE

    if ( piece_block )
        bc->release( piece_block );

    bc->release( piecemap_block, false );
}

void
FileBlockLocal :: free_map( UINT64 pos, UINT32 len )
{
    PieceMapEntry * pme;
    BlockCacheBlock * piecemap_block;

    //   fetch old piece-map
    piecemap_block = bc->get( pos, len );

    pme = (PieceMapEntry *) piecemap_block->get_ptr();

    //   foreach entry in piece-map
    //      map.free
    while (pme->offset.get() != 0)
    {
        printf("freeing piece at offset %lld\n", pme->offset.get());
        map.free_offset( pme->offset.get() );
        pme++;
    }

    bc->release( piecemap_block, false );

    //   free piece-map
    printf("freeing piece-map at offset %lld\n", pos);
    map.free_offset( pos );
}

//static
void
FileBlockLocal :: store_map( Extents * m, BlockCache * bc,
                             UINT64_t * pos, UINT32_t * len )
{
    PieceMapEntry * pme;
    BlockCacheBlock * piecemap_block;

    // calculate how many pieces are needed for the new map.

    int num_used = m->get_count_used();
    int num_free = m->get_count_free();

    // increase num_free to account for a zero entry at the end.
    num_free ++;

    printf( "num_used = %d   num_free = %d\n", num_used, num_free );

    // used entries need size plus id (8 bytes) while 
    // free entries need only size (4 bytes).
    int bytes_needed = num_used * 8 + num_free * 4;
    int num_pieces = ((bytes_needed-1) / PIECE_SIZE) + 1;
    int piece_map_bytes = (num_pieces + 1) * sizeof(PieceMapEntry);

    printf( "bytes_needed = %d\n", bytes_needed );
    printf( "num_pieces = %d\n", num_pieces );
    printf( "piece_map_bytes = %d\n", piece_map_bytes );

    // now we need to alloc all the space for everything;
    // this will change the size of the extent map, therefore
    // now that we roughly know how many pieces are required,
    // recalculate with this in mind, just in case the number
    // of needed pieces changed.
    num_used += num_pieces + 2;
    bytes_needed = num_used * 8 + num_free * 4;
    num_pieces = ((bytes_needed-1) / PIECE_SIZE) + 1;
    int bytes_in_last_piece = bytes_needed - ((num_pieces-1) * PIECE_SIZE);
    piece_map_bytes = (num_pieces + 1) * sizeof(PieceMapEntry);

    printf( "recalculated bytes_needed = %d\n", bytes_needed );
    printf( "recalculated num_pieces = %d\n", num_pieces );
    printf( "recalculated piece_map_bytes = %d\n", piece_map_bytes );
    printf( "bytes_in_last_piece = %d\n", bytes_in_last_piece );

    // m->alloc a region for the new piece-map.
    Extent * e = m->alloc( piece_map_bytes );
    piecemap_block = bc->get( e->offset, e->size, true );
    printf( "piecemap_block is at %lld\n", e->offset );
    pme = (PieceMapEntry *) piecemap_block->get_ptr();

    pos->set( e->offset );
    len->set( e->size );

    UINT32_t * pieces[num_pieces];
    BlockCacheBlock * bcbs[num_pieces];

    int i, j;
    // foreach entry in piece-map
    //   m->alloc space for a new piece
    for (i=0; i < num_pieces; i++)
    {
        UINT32 size = i==(num_pieces-1) ? bytes_in_last_piece : PIECE_SIZE;
        Extent * e = m->alloc( size );
        printf( "piece %d is at %lld, size %d\n", i, e->offset, size );
        pme[i].offset.set( e->offset );
        pme[i].len.set( size );
        bcbs[i] = bc->get( e->offset, size, true );
        pieces[i] = (UINT32_t*) bcbs[i]->get_ptr();
    }

    i = 0;
    j = 0;

#define ADVANCE()                               \
    if (++j == (PIECE_SIZE/sizeof(UINT32_t)))   \
    {                                           \
        j = 0;                                  \
        i++;                                    \
    }

    // foreach entry in m
    //   populate encoded entry into piece

    for ( e = m->get_head(); e; e = m->get_next(e))
    {
        pieces[i][j].set( (e->used << 31) + e->size );
        printf( "piece %d entry %d gets size value %#x\n", 
                i, j, pieces[i][j].get());
        ADVANCE();
        if (e->used)
        {
            pieces[i][j].set( e->id );
            printf( "piece %d entry %d gets id value %#x\n", 
                    i, j, pieces[i][j].get());
            ADVANCE();
        }
    }

#undef ADVANCE

    // release all pieces
    for (i=0; i < num_pieces; i++)
        bc->release( bcbs[i], true );
    bc->release( piecemap_block, true );
}
