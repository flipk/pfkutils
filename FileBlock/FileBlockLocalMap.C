
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

/** the size of each block allocated to store pieces of the
 * extent map. */
#define PIECE_SIZE 8192

//static
void
FileBlockLocal :: store_map( Extents * m, BlockCache * bc,
                             off_t * first_piece_pos,
                             UINT32 * first_piece_len )
{
    int count_used, count_free;

    count_used = m->get_count_used();
    count_free = m->get_count_free();

    // calculate how many bytes needed to store the 
    // map itself; this does NOT count the linked-list
    // stuff at the end of each piece.  used extents store
    // the size and id; free extents store only the size.
    int bytes_needed = count_used * 8 + count_free * 4;

    // calculate number of pieces required; this accounts
    // for 12 bytes of linked-list stuff at the end of each
    // piece.  also always add 1 in case the allocation of 
    // pieces from the map causes the map to grow over a piece
    // boundary.
    int pieces_needed = (bytes_needed / (PIECE_SIZE / 12)) + 1;

    // xxx
    
}
