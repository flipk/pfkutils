
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


/** \page FileBlock FileBlock object

This is the guts and gore/core of file space allocation management.

\dot 

digraph FileBlockStructure {
  graph [rankdir=LR];
  node [shape=record, fontname=Helvetica, fontsize=10];
  edge [arrowhead="open", style="solid"];

  Alloc     [label="Alloc/Free"        URL="\ref FileBlockAccess" ];
  AUIDmgmt  [label="AUID Management"   URL="\ref AUIDMGMT"        ];
  AUIDtab   [label="AUID Table"        URL="\ref AUIDTable"       ];
  AUIDstack [label="AUID Stack"        URL="\ref AUIDTable"       ];
  AUNmgmt   [label="AUN Management"    URL="\ref AUNMGMT"         ];
  Bucket    [label="Bucket Management" URL="\ref AUNBuckets"      ];

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

</ul>

*/

/** \page FileBlock FileBlock object

 \section FileBlockAccess FileBlock Access Methods: Allocation

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

*/

/** \page FileBlock FileBlock object

 \section FileBlockAccessR FileBlock Access Methods: Retrieval

The second phase is to actually access that region.  This is done with two
more methods, FileBlockInterface::get and FileBlockInterface::release.  The
first takes an AUID number created by the alloc method, and returns a
FileBlock, which contains a memory buffer the user can read and modify.
The release method takes this FileBlock object back, and ensures the memory
buffer's contents are synchronized with the actual disk file.

See the section \ref Templates for an example of the access methods.

*/

/** \page FileBlock FileBlock object

 \section AUNMGMT  AUN Management

The first level of management in the file is lists of regions.  Every 
region is on a global ordered list.  When a region is freed, this allows
fast lookups of previous and successive regions, to determine if coalescing
of free space is possible.

Each region encodes the following information:

<ul>
<li> The AUN of the previous region.  If this is the first region in the
     file, this value is zero.
<li> The size of the region, in AUs.  This field is 31-bits, therefore
     the largest region which can be expressed in this field is 2^31*32=64GB.
     The size value of 0 is reserved for the last region in the file, also
     known as an end-of-file marker.
<li> There is one bit for a used/free indicator.
<li> A used region contains:
   <ul>
   <li> The AUID value used to represent this item.  The value of 0 is
        reserved to indicate a stack L2 or L3 page.
   </ul>
<li> A free region contains:
   <ul>
   <li> A bucket_prev pointer for the bucket linked list.
   <li> A bucket_next pointer for the bucket linked list.
   </ul>
</ul>

Note that an AUN of 0 (zero) indicates an invalid AUN or an end-of-list
marker.

*/

/** \page FileBlock FileBlock object

 \section AUNBuckets  AUN Bucket Lists

Every free region is on a bucket list.  There are 2048 bucket lists,
each one representing regions of a particular size, from 1 to 2047 AUs
in size.  Each bucket list is a doubly-linked list.

There is also a bucket bitmap.  Each bucket list head pointer has a
corresponding bit in this bitmap.  If the list is empty, the bit is zero.
If the list is nonempty, the bit is one.

When an allocation of a particular size is requested, that specific
bucket is checked first for a region of exactly the right size.  If
that bucket list is empty, the search proceeds up to the next bucket
list for regions of the next size.  The bucket bitmap is used to
accelerate this search.  The final bucket list (2048) is reserved for
an end-of-file marker, whose size is considered infinite.

When a matching free region is found (even if it is the infinite-sized
end-of-file marker), this free region is split into two regions: a
used region for the allocation, and another region containing the
remainder of the original free region.  (For the end-of-file marker,
the new region continues to be marked as infinite.)

When a region is freed, the next and previous regions are checked to
see if they are also free.  If they are, the current region is
combined with them to make one contiguous free region.

The resulting free region is then inserted into the head of a bucket
list, corresponding to the size of the region.

*/

/** \page FileBlock FileBlock object

 \section AUIDMGMT  AUID Management

The next level of management is management of the AUID identifiers. 
Every data block in a file must be identified with a unique identifier
which does not change if the data is moved to a different position in 
the file.  Therefore, there must be a mechanism to store and retrieve
the translation of identifier to position.

This is done with a set of tables.  There are two tables, one for managing
the AUID-to-AUN translation, and one for managing free AUIDs.  The first 
is known as the AUID table, the second is known as the AUID free-stack.

The AUID is the index in the first table.  AUIDs are allocated bottom
up, starting at 1.  Zero is an invalid AUID.  The value auid_top in
the InfoBlock indicates the next available AUID value.  When an AUID
is freed, the entry in the table indexed by that AUID is set to zero,
creating a hole in the table.  The AUID value is then added to the
AUID free-stack for reclamation the next time an AUID must be
allocated.

The index in the free-stack is the value auid_stack_top in the InfoBlock.
This corresponds to the next slot in the free-stack which could be written
with a new AUID.  If auid_stack_top is zero, the stack is empty.  This also
implies that every AUID from 1 thru auid_top is currently in use.

When an AUID allocation is requested, the free-stack is checked first.
If the stack is not empty, the top value is taken and used.  If the
stack is empty, then auid_top is incremented and its previous value is
the AUID returned.

When an AUID is freed, the entry in the AUID table is set to zero and the
AUID is added to the pointer stack.

*/


/** \page FileBlock FileBlock object

 \section AUIDTable  AUID Three-level table

A given data file could have thousands or millions or even a billion
used regions, and thus a great many AUIDs to manage.  Thus, it is not
feasable to implement the AUID table and free-stack as flat tables.

Thus they are implemented as a three-level table system.  A 32-bit
value is divided into three fields: the top 12 bits become a level 1
(L1) index, the next 10 bits are the L2 index, and the bottom 10 are
the L3 index.

The FileHeader structure includes the two L1 tables.  Each entry in
each L1 table is an AUN identifying the location of an L2 table.

Each entry in the L2 table is an AUN specifying a location of an L3
table.

For the AUID table, each entry in the L3 table is an AUN specifying
the location of the region identified by the AUID value.

For the AUID free stack, each entry represents an AUID value currently
unused in the AUID table.

L2 and L3 tables are allocated dynamically as required by calling AUN
allocation functions.

\note There is no attempt to reclaim L2 or L3 tables if the free-stack
size decreases, nor is there any attempt to reclaim tables from the
AUID table if large blocks of contiguous AUIDs are freed.

\note L2 and L3 tables are allocated using the AUN management interface,
but the AUID field is never populated.  The AUID value is set to 0 for
all L2 and L3 tables.  L2 and L3 tables are located using AUN values
instead of AUID values.  This is because the tables themselves provide
the translation of AUID-to-AUN, thus AUID cannot be used.

*/

/** \page FileBlock FileBlock object

\section FileBlockDataInfoBlock DataInfoBlocks

An application might like to store a bookmark of some kind, informing
where to start looking for data.  For instance if the file is being
used to store a B-tree, the btree may require an info block containing
information such as the order of the tree and a pointer to the root
node.  It may also be convenient to store more than one set of data in
the same file.  For example it may be reasonable to store multiple
independent B-trees in the same file.  This could be as easy as having
two separate info blocks pointing to two separate root nodes.

Thus the API provides a mechanism to associate a text string with an
AUID value.  The application provides the string name and an AUID and
an association as formed.  The next time the file is opened, the
string name can be used to search for the AUID.

To enter a file block association into the file, the user specifies
the unique string and provides an AUID of the info block when calling
FileBlockInterface::set_data_info_block.

To retrieve this information later, the user specifies the unique
string and then calls FileBlockInterface::get_data_info_block.

If this information is no longer needed, the user should delete the
association by calling FileBlockInterface::del_data_info_block.

*/

/** \page FileBlock FileBlock object

 \section Compacting

Possible implementations include:

<ul>
<li>  Recreating the file from scratch

This requires halting access to the file and creating an entirely new
file, walking the original file in block order and simply
tail-allocating from the new file, guaranteeing the new file is
completely packed.  While this guarantees perfect packing and
excellent performance (due to the linear nature of both the reader and
the writer) it unfortunately requires the access to the file to be
halted-- an unacceptable step for any program intended to run
continuously for long periods.

Performance with this method would be excellent if the source and
destination files were on different disk spindles, due to the mostly
linear access to the two files, but performance would be very poor if
they were on the same spindle due to repeated seeking between the two
files.

<li>  In-place downshifting

In this method, the file is walked from beginning to end.  Each time a
free region is encountered, a used-region following it is moved down
into the free space.  The free space is then pushed up.  Whenever a
free region is encountered, it is coalesced with the free space being
pushed up. If this is done through the entire file, all the free space
will be eventually coalesced with the end-of-file marker and the 
overall file size will be reduced.

This is complicated by the AUID free-stack L2/L3 tables.  For any
other used-block, the AUID translation table can be modified to the
new AUN value when the block is moved.  But L2 tables are identified
only by an AUN value stored in the L1 table, and L3 tables are
identified only by an AUN value stored in the L2 table.  There are no
AUIDs to modify, so for example when an L2 table is moved, the
corresponding L1 entry must be modified.

To address this problem, the compaction algorithm will need to first
walk the list of all L2 and L3 tables for both the AUID table and the
free stack, and collect a list in memory.  When an AUID of zero is
encountered in a used-block, the list should be consulted, so that
when the table is moved, the proper AUN value in the parent table can
be updated.

This algorithm would process the file in a linear fashion.  This is
not a good scenario for the PageCache, as the cache would be filling
linearly from one end and doing write-backs linearly from the other
end.  If the PageCache were made sufficiently large, performance
degradation could be kept to a minimum if well-timed cache flushes
were performed, guaranteeing that a large number of dirty pages would
be written out in a linear (and thus high-speed) fashion.

<li> Bucket matching

In this method, the header data in a used-region will need to be modified
to include a used-bucket list, so that even used-regions are on a list
corresponding to bucket size.  Then, the algorithm is as follows:

<ul>
<li> For each bucket size, 1-2047:
   <ul>
   <li>  Read into memory a list of all free AUNs of this bucket size
   <li>  Read into memory a list of all used AUNs of this bucket size
   <li>  Sort the free AUNs in increasing order
   <li>  Sort the used AUNs in decreasing order
   <li>  For index counts from 1 to size of smaller list
      <ul>
      <li> move the first used region on the used list to the
           first free region on the free list, and dequeue from both lists.
      <li> continue this loop until either:
         <ul>
         <li> one of the lists is empty, or
         <li> the next free AUN is larger than the next used AUN.
         </ul>
      </ul>
   </ul>
</ul>

This method has a significant performance impact on the PageCache, as it
would cause random seeking all over the file to build the bucket lists,
and then seeking from beginning to end and back again repeatedly as
blocks are moved down.

</ul>

See also: \ref FileBlockBSTExample

Next: \ref BtreeStructure

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
