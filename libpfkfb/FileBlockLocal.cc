
/*
    This file is part of the "pfkutils" tools written by Phil Knaack
    (pfk@pfk.org).
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

/** \file FileBlockLocal.cc
 * \brief Local implementation of FileBlock_iface.  This file contains
 *   only initialization and destruction methods.
 * \author Phillip F Knaack
 */


/** \page FileBlock FileBlock object

This is the guts and gore/core of file space allocation management.

\dot 

digraph FileBlockStructure {
  graph [rankdir=LR];
  node [shape=record, fontname=Helvetica, fontsize=10];
  edge [arrowhead="open", style="solid"];

  Alloc     [label="Alloc/\nRetrieval"  URL="\ref FileBlockAccess" ];
  AUIDmgmt  [label="AUID\nMgt"          URL="\ref AUIDMGMT"        ];
  AUIDtab   [label="AUID\nTable"        URL="\ref AUIDTable"       ];
  AUIDstack [label="AUID\nStack"        URL="\ref AUIDTable"       ];
  AUNmgmt   [label="AUN\nMgt"           URL="\ref AUNMGMT"         ];
  Bucket    [label="Bucket\nMgt"        URL="\ref AUNBuckets"      ];

  Alloc     -> AUIDmgmt  ;
  Alloc     -> AUNmgmt   ;
  AUIDmgmt  -> AUIDtab   ;
  AUIDmgmt  -> AUIDstack ;
  AUIDtab   -> AUNmgmt   ;
  AUIDstack -> AUNmgmt   ;
  AUNmgmt   -> Bucket    ;
}

\enddot

 \section FBDefinitions FileBlock term definitions

<ul>
<li> AU : Allocation Unit

This is the fundamental unit of accessing a disk.  An AU is 32 bytes in size.

<li> AUN : Allocation Unit Number (FB_AUN_T)

This is an identifer for an AU which identifies the AU by its position in
the file.  Since an AU is 32 bytes in size, the position in the file can
be calculated from an AUN:  pos=(AUN*32).

This data type (FB_AUN_T) is completely internal to the FileBlockLocal 
implementation, and is not available to applications.

The AUN value of zero (which, technically, represents the beginning of
the file) is an invalid value, reserved for indicating end-of-list,
etc.

\note  Since the AUN is a 32-bit value, the size of a file is limited to
2^32 * 32 = 137438953472 = 128 GB.

<li> AUID : Allocation Unit Identifier (FB_AUID_T)

This is also an identifer of an AU, but this identifier is completely 
independent of the position in the file.  In order to allow the FileBlock
API to perform transparent compactions of the file, but still allow the 
application to store identifiers in their own data structures, it is 
necessary to separate the identifier from the position in the file.  Then,
the API can feel free to move a unit to another place in the file without
informing the application that it must change its identifier.

There is a translation mechanism in the file providing a lookup table from
AUID to AUN so that a block can be located quickly.  If the API decides to
move a unit in order to compact the file, the lookup table is changed but
the application continues to use the same AUID.

An AUID value of zero is invalid, and reserved for use in identifying
blocks in the file used for the AUID lookup tables.

</ul>

Fileblock sections:

<ul>
<li>  \ref FileBlockAccess
<li>  \ref FileBlockAccessR
<li>  \ref AUNMGMT
<li>  \ref AUNBuckets
<li>  \ref AUIDMGMT
<li>  \ref AUIDTable
<li>  \ref FileBlockCompacting
</ul>

*/


#include "FileBlockLocal.h"

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

//virtual
void
FileBlockLocal :: get_stats( FileBlockStats * stats )
{
    stats->au_size      = AU_SIZE;
    stats->used_aus     = fh.d->info.used_aus    .get();
    stats->free_aus     = fh.d->info.free_aus    .get();
    stats->used_regions = fh.d->info.used_extents.get();
    stats->free_regions = fh.d->info.free_extents.get();
    stats->num_aus      = fh.d->info.num_aus     .get();
}
