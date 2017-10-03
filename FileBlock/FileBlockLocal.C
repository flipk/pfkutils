
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
    /** \todo */
}

//virtual
FileBlockLocal :: ~FileBlockLocal( void )
{
    /** \todo */
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
