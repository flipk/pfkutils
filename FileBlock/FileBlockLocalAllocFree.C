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

/** \file FileBlockLocalAllocFree.C
 * \brief User interface functions for allocation, freeing, and reallocation.
 * \author Phillip F Knaack
 */

#include "FileBlockLocal.H"

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
#if AUIDS_IN_USED
    au.d->auid(auid);
#endif

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

    aun = alloc_aun(new_size);
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
