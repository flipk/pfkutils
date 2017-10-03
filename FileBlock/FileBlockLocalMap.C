
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

//static
void
FileBlockLocal :: store_map( Extents * m, BlockCache * bc,
                             UINT64_t * pos, UINT32_t * len )
{

    // first, if *pos is nonzero, free up all space in the map
    // consumed by the old map:
    //   fetch old piece-map
    //   foreach entry in piece-map
    //      m->free

    // then, calculate how many pieces are needed for the new map.
    // m->alloc a region for the new piece-map.
    // foreach entry in piece-map
    //   m->alloc space for a new piece
    // foreach entry in m
    //   populate encoded entry into piece
    // release all pieces

}
