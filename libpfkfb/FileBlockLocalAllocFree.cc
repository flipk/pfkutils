/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/

/** \file FileBlockLocalAllocFree.cc
 * \brief User interface functions for allocation, freeing, and reallocation.
 */


/** \page FileBlockAccess FileBlock Access Methods: Allocation

There are two separate phases in accessing a FileBlock file.  First there
is allocation management, in which there are three methods:
 FileBlockInterface::alloc, FileBlockInterface::realloc, and
FileBlockInterface::free.

These functions deal only in FileBlock AUID numbers.  An AUID does not
exist until an alloc call, and ceases to exist after a free call.
When allocating an AUID, the size must also be specified.  After the
alloc call, there now exists a region within the disk file of the
specified size which is associated with this ID.

The realloc method is basically a free followed by another alloc of
the new size, except that the new allocation has the same AUID number as
the old one.  Note that it will copy the data from the old region to the
new region; if the new size is smaller, the data is truncated, but if the
new size is bigger, the new space at the end of the allocation is memset
to zeroes.

Next: \ref FileBlockAccessR

*/



#include "FileBlockLocal.h"

#include <stdlib.h>

//virtual
FB_AUID_T
FileBlockLocal :: alloc( int size )
{
    FB_AUN_T  aun;
    FB_AUID_T auid;

    if (size > FILE_BLOCK_MAXIMUM_ALLOCATION_SIZE)
    {
        std::cerr << "FileBlockLocal::alloc : allocation of "
                  << size << " attempted is larger than max "
                  << "supported allocation size "
                  << FILE_BLOCK_MAXIMUM_ALLOCATION_SIZE
                  << std::endl;
        return 0;
    }

    AUHead au(bc);

    aun = alloc_aun( &au, size );
    if (aun == 0)
        return 0;

    auid = alloc_auid( aun );
    au.d->auid(auid);

    return auid;
}

//virtual
void
FileBlockLocal :: free( FB_AUID_T auid )
{
    FB_AUN_T aun;

    aun = translate_auid( auid );
    if (aun == 0)
        return;

    free_aun( aun );
    free_auid( auid );
}

//virtual
FB_AUID_T
FileBlockLocal :: realloc( FB_AUID_T auid, int new_size )
{
    return realloc(auid, 0, new_size);
}

FB_AUID_T
FileBlockLocal :: realloc( FB_AUID_T auid, FB_AUN_T to_aun,
                           int new_size )
{
    FB_AUN_T aun;
    uint8_t * buffer;
    int buflen;

    aun = translate_auid( auid );
    if (aun == 0)
        return 0;

    FileBlock * fb;
    fb = get_aun(aun);
    if (!fb)
        return 0;

    buflen = fb->get_size();
    if (new_size == 0)
        new_size = buflen;
    if (buflen >= new_size)
    {
        buflen = new_size;
        buffer = new uint8_t[ buflen ];
        memcpy(buffer, fb->get_ptr(), buflen);
    }
    else
    {
        buffer = new uint8_t[ new_size ];
        memcpy(buffer, fb->get_ptr(), buflen);
        memset(buffer + buflen, 0, new_size - buflen);
        buflen = new_size;
    }
    release(fb,false);

    free_aun(aun);

    {
        AUHead au(bc);
        aun = alloc_aun(to_aun, &au, new_size);
        au.d->auid(auid);
    }
    fb = get_aun(aun,true);
    if (!fb)
    {
        fprintf(stderr, "FileBlockLocal :: realloc: crap!!\n");
        /*DEBUGME*/
        exit(1);
    }
    memcpy(fb->get_ptr(), buffer, buflen);
    release(fb);
    rename_auid(auid, aun);

    delete[] buffer;

    return auid;
}
