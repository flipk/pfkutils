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

/** \file FileBlockLocalCompact.C
 * \brief Implementation of compaction algorithms.
 * \author Phillip F Knaack
 */


/** \page FileBlockCompacting FileBlock Compacting

At this time, compaction is not yet implemented, mostly because a 
design has not yet been finalized. Possible implementations include:

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

<li>  Unpeeling + Downshifting

<ul>
<li> walk AUID and free-stack L1 tables and L2 tables, gathering
     a list of all L2 and L3 table AUNs; also record the location
     in the parent table where its AUN is stored.
<li> while number of free AUs is greator than 5% of the file
   <ul>
   <li> find the last used-block in the file
   <li> attempt a realloc 
   <li> if the new position in the file is before the
        position of this block
      <ul>
      <li> move the block, check AUID
      <li> if AUID is zero
         <ul>
         <li> this is an L2 or L3 table; find the AUN in the list,
              and modify the parent to point to the new AUN.
         </ul>
      <li> else
         <ul>
         <li> rename the AUID to the new AUN.
         </ul>
      </ul>
   <li> else
      <ul>
      <li> start at the beginning of the file, performing Downshifting
           until a free block large enough to contain the last block has
           been coalesced.
      </ul>
   </ul>
</ul>

</ul>

See also: \ref FileBlockBSTExample

Next: \ref BtreeStructure

*/


#include "FileBlockLocal.H"

#include <stdlib.h>

//virtual
void
FileBlockLocal :: compact( bool full )
{
    // not yet supported
}
