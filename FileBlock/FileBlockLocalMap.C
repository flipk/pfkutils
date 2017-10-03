/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

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
FileBlockLocal :: load_map( FileBlockLocalHeader * fbh )
{
    PieceMapEntry * pme;
    BlockCacheBlock * piecemap_block;

    // can't use BlockCacheT because pme has to be an array

    piecemap_block = bc->get( fbh->piece_map_start.get(),
                              fbh->piece_map_len.get() );

    pme = (PieceMapEntry *) piecemap_block->get_ptr();

    UINT64  current_pos = 0;
    UINT64  piece_offset;
    UINT32  piece_len = 0;
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
            map.add( current_pos, size, val );
        }
        else
        {
            // add a free entry
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
FileBlockLocal :: free_map( FileBlockLocalHeader * fbh )
{
    PieceMapEntry * pme;
    BlockCacheBlock * piecemap_block;

    // can't use BlockCacheT because pme must be an array.

    //   fetch old piece-map
    piecemap_block = bc->get( fbh->piece_map_start.get(),
                              fbh->piece_map_len.get() );

    pme = (PieceMapEntry *) piecemap_block->get_ptr();

    //   foreach entry in piece-map
    //      map.free
    while (pme->offset.get() != 0)
    {
        map.free_id( pme->id.get() );
        pme++;
    }

    bc->release( piecemap_block, false );

    //   free piece-map
    map.free_id( fbh->piece_map_id.get() );
}

//static
void
FileBlockLocal :: store_map( Extents * m, BlockCache * bc,
                             FileBlockLocalHeader * fbh )
{
    PieceMapEntry * pme;
    BlockCacheBlock * piecemap_block;

    // calculate how many pieces are needed for the new map.

    int num_used = m->get_count_used();
    int num_free = m->get_count_free();

    // increase num_free to account for a zero entry at the end.
    num_free ++;

    // used entries need size plus id (8 bytes) while 
    // free entries need only size (4 bytes).
    int bytes_needed = num_used * 8 + num_free * 4;
    int num_pieces = ((bytes_needed-1) / PIECE_SIZE) + 1;
    int piece_map_bytes = (num_pieces + 1) * sizeof(PieceMapEntry);

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

    // m->alloc a region for the new piece-map.
    Extent * e = m->alloc( piece_map_bytes );
    piecemap_block = bc->get( e->offset, e->size, true );
    pme = (PieceMapEntry *) piecemap_block->get_ptr();

    fbh->piece_map_start.set( e->offset );
    fbh->piece_map_id.set( e->id );
    fbh->piece_map_len.set( e->size );

    UINT32_t * pieces[num_pieces];
    BlockCacheBlock * bcbs[num_pieces];

    int i, j;
    // foreach entry in piece-map
    //   m->alloc space for a new piece
    for (i=0; i < num_pieces; i++)
    {
        UINT32 size = i==(num_pieces-1) ? bytes_in_last_piece : PIECE_SIZE;
        Extent * e = m->alloc( size );
        pme[i].offset.set( e->offset );
        pme[i].id.set( e->id );
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
        ADVANCE();
        if (e->used)
        {
            pieces[i][j].set( e->id );
            ADVANCE();
        }
    }

#undef ADVANCE

    // release all pieces
    for (i=0; i < num_pieces; i++)
        bc->release( bcbs[i], true );
    bc->release( piecemap_block, true );
}
