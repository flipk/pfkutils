/*
    This file is part of the "pfkutils" tools written by Phil Knaack
    (pknaack1@netscape.net).
    Copyright (C) 2008  Phillip F Knaack

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/** \file FileBlockLocalAllocFree.cc
 * \brief User interface functions for allocation, freeing, and reallocation.
 * \author Phillip F Knaack
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
    UCHAR * buffer;
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
        buffer = new UCHAR[ buflen ];
        memcpy(buffer, fb->get_ptr(), buflen);
    }
    else
    {
        buffer = new UCHAR[ new_size ];
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
