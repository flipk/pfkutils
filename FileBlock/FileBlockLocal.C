
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

/** \file FileBlockLocal.C
 * \brief Local implementation of FileBlock_iface.  This file contains
 *   only initialization and destruction methods.
 * \author Phillip F Knaack
 */

#include "FileBlockLocal.H"

#include <stdlib.h>

FileBlockLocal :: FileBlockLocal( BlockCache * _bc)
    : fh(_bc)
{
    bc = _bc;
    fh.get();
}

/*virtual*/
FileBlockLocal :: ~FileBlockLocal(void)
{
    flush();
    fh.release();
    delete bc;
}

//static
bool
FileBlockLocal :: valid_file( BlockCache * bc )
{
    FileHeader   _fh(bc);
    if (_fh.d->info.signature.get() != InfoBlock::SIGNATURE)
        return false;
    return true;
}

//static
void
FileBlockLocal :: init_file( BlockCache * bc )
{
    FileHeader    _fh(bc);
    AUHead        head(bc);
    FB_AUN_T      first_user_au;
    int i;

    if (!_fh.get(true))
        return;

    first_user_au = SIZE_TO_AUS(sizeof(_FileHeader));

    _fh.d->info.signature.set(InfoBlock::SIGNATURE);
    _fh.d->info.free_aus.set(0);
    _fh.d->info.used_aus.set(0);
    _fh.d->info.used_extents.set(0);
    _fh.d->info.free_extents.set(1);
    _fh.d->info.first_au.set(first_user_au);
    _fh.d->info.num_aus.set(first_user_au);

    for (i=0; i < DataInfoPtrs::MAX_DATA_INFOS; i++)
        _fh.d->data_info_ptrs.ptrs[i].set(0);
    
    for (i=0; i < BucketList::NUM_BUCKETS; i++)
        _fh.d->bucket_list.list_head[i].set(0);

    _fh.d->bucket_list
        .list_head[BucketList::NUM_BUCKETS-1]
        .set( first_user_au );
    _fh.d->bucket_bitmap.set_bit(BucketList::NUM_BUCKETS-1,true);

    for (i=0; i < AuidL1Tab::L1_ENTRIES; i++)
    {
        _fh.d->auid_l1      .entries[i].set(0);
        _fh.d->auid_stack_l1.entries[i].set(0);
    }
    _fh.d->info.auid_top.set(1);
    _fh.d->info.auid_stack_top.set(0);

    head.get(first_user_au, true);

    head.d->prev.set( 0 );
    head.d->size( 0 );  // a size of 0 means infinite (last entry)
    head.d->used( false );
    head.d->bucket_next( 0 );
    head.d->bucket_prev( 0 );
}
