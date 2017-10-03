
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

/** \file FileBlockLocalDIB.C
 * \brief Everything related to data info blocks: set, fetch, and delete.
 * \author Phillip F Knaack
 */

#include "FileBlockLocal.H"

#include <stdlib.h>

//virtual
FB_AUID_T
FileBlockLocal :: get_data_info_block( char * info_name )
{
    int i;

    DataInfoBlock dib(this);

    for (i=0; i < DataInfoPtrs::MAX_DATA_INFOS; i++)
    {
        FB_AUID_T  dip = fh.d->data_info_ptrs.ptrs[i].get();
        if (dip == 0)
            continue;
        if (!dib.get(dip))
            continue;
        if (strncmp(info_name,
                    dib.d->info_name,
                    _DataInfoBlock::MAX_INFO_NAME) == 0)
        {
            return dib.d->info_auid.get();
        }
    }

    return 0;
}

//virtual
void
FileBlockLocal :: set_data_info_block( FB_AUID_T auid, char *info_name )
{
    int i;

    for (i=0; i < DataInfoPtrs::MAX_DATA_INFOS; i++)
    {
        FB_AUID_T  dip = fh.d->data_info_ptrs.ptrs[i].get();
        if (dip == 0)
            break;
    }
    if (i == DataInfoPtrs::MAX_DATA_INFOS)
    {
        fprintf(stderr, "out of data info blocks!!\n");
        exit(1);
    }
    DataInfoBlock dib(this);
    FB_AUID_T dip = dib.alloc();
    if (!dib.get(dip,true))
    {
        fprintf(stderr, "FileBlockLocal :: set_data_info_block: crap!\n");
        /*DEBUGME*/
        exit(1);
    }

    dib.d->info_auid.set(auid);
    strncpy(dib.d->info_name, info_name, _DataInfoBlock::MAX_INFO_NAME);
    dib.d->info_name[_DataInfoBlock::MAX_INFO_NAME-1] = 0;
    dib.mark_dirty();

    fh.d->data_info_ptrs.ptrs[i].set(dip);
    fh.mark_dirty();
}

//virtual
void
FileBlockLocal :: del_data_info_block( char * info_name )
{
    int i;
    FB_AUID_T  dip = 0;

    DataInfoBlock dib(this);

    for (i=0; i < DataInfoPtrs::MAX_DATA_INFOS; i++)
    {
        dip = fh.d->data_info_ptrs.ptrs[i].get();
        if (dip == 0)
            continue;
        if (!dib.get(dip))
            continue;
        if (strncmp(info_name,
                    dib.d->info_name,
                    _DataInfoBlock::MAX_INFO_NAME) == 0)
        {
            break;
        }
    }

    if (dip == 0)
        return;

    free( dip );
    fh.d->data_info_ptrs.ptrs[i].set(0);
}
