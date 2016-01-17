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

/** \file FileBlockLocalAuid.cc
 * \brief All methods for AUID management: allocation, free, stack, and
 *     L1/L2/L3 table maintanence.
 * \author Phillip F Knaack
 */


/** \page AUIDMGMT  FileBlock AUID Management

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

Next: \ref AUIDTable

*/


/** \page AUIDTable  FileBlock AUID Three-level table


A given data file could have thousands or millions or even a billion
used regions, and thus a great many AUIDs to manage.  Thus, it is not
feasable to implement the AUID table and free-stack as flat tables.
They are implemented as a three-level table system.  

Both tables have identical structures:

<ul>
<li> Both tables are indexed by a 32-bit value:
   <ul>
   <li> The AUID table is indexed by the AUID number.
   <li> The free-stack is indexed by a "stack_top" value.
   </ul>
<li> Each table is a three-level heirarchical table:
   <ul>
   <li> The first level is called the "L1" table and is found in the
        FileHeader structure.  The L1 table is 4096 entries, indexed
        by the top 12 bits of the 32-bit value used to index this
        table.  The value at each entry in this L1 table is an AUN
        number used to find an L2 table.  If this value is 0, no L2
        table exists.
   <li> The L2 table is dynamically allocated only as needed.  It is
        1024 entries long, and indexed using the next 10 bits of the
        index value.  The resulting value is an AUN of the L3 table,
        if one exists.
   <li> The L3 table is also dynamically allocated, also 1024 entries
        long, and indexed using the last 10 bits of the index value.
        In the AUID table, the resulting value is the AUN location of
        the block identified by the AUID value; in the free-stack, the
        resulting value is an AUID value which is currently free in
        the AUID table.
   </ul> 
</ul>

L2 and L3 tables are allocated dynamically as required by calling AUN
allocation functions.

\note There is no attempt to reclaim L2 or L3 tables if the free-stack
size decreases, nor is there any attempt to reclaim tables from the
AUID table if large blocks of contiguous AUIDs are freed.

\note L2 and L3 tables are allocated using the AUN management
interface, but the AUID field is never populated.  The AUID value is
set to 0 for all L2 and L3 tables.  L2 and L3 tables are located using
AUN values instead of AUID values.  This is because the tables
themselves provide the translation of AUID-to-AUN, thus AUID cannot be
used.  (This presents a special challenge for file compaction,
described in the next section.)

Next: \ref FileBlockCompacting

*/


#include "FileBlockLocal.h"

#include <stdlib.h>

FB_AUID_T
FileBlockLocal :: alloc_auid( FB_AUN_T aun )
{
    FB_AUID_T auid = 0;
    uint32_t stack_top;

    stack_top = fh.d->info.auid_stack_top.get();
    if (stack_top != 0)
    {
        stack_top --;
        auid = lookup_stack(stack_top);
        write_stack(stack_top,0);
        rename_auid(auid,aun);
        fh.d->info.auid_stack_top.set(stack_top);
        fh.mark_dirty();
        return auid;
    }

    // if free stack is empty, need to alloc a new auid.

    auid = fh.d->info.auid_top.get();
    fh.d->info.auid_top.set(auid+1);
    fh.mark_dirty();

    int l1_index = AuidL1Tab ::auid_to_l1_index(auid);
    int l2_index = AuidL23Tab::auid_to_l2_index(auid);
    int l3_index = AuidL23Tab::auid_to_l3_index(auid);

    FB_AUN_T l2_table_aun = fh.d->auid_l1.entries[l1_index].get();
    FileBlock   * l2fb;
    if (l2_table_aun == 0)
    {
        // need to allocate a new l2 table!

        l2_table_aun = alloc_aun( sizeof(AuidL23Tab) );
        fh.d->auid_l1.entries[l1_index].set(l2_table_aun);
        fh.mark_dirty();
        l2fb = get_aun( l2_table_aun, true );
    }
    else
        l2fb = get_aun( l2_table_aun );
    AuidL23Tab  * l2tab = (AuidL23Tab*)l2fb->get_ptr();

    FB_AUN_T l3_table_aun = l2tab->entries[l2_index].get();
    FileBlock   * l3fb;
    if (l3_table_aun == 0)
    {
        // need to alloc a new l3 table!

        l3_table_aun = alloc_aun( sizeof(AuidL23Tab) );
        l2tab->entries[l2_index].set( l3_table_aun );
        l2fb->mark_dirty();
        l3fb = get_aun( l3_table_aun, true );
    }
    else
        l3fb = get_aun( l3_table_aun );
    AuidL23Tab  * l3tab = (AuidL23Tab*)l3fb->get_ptr();

    l3tab->entries[l3_index].set( aun );
    l3fb->mark_dirty();

    release( l2fb );
    release( l3fb );

    return auid;
}

void
FileBlockLocal :: rename_auid( FB_AUID_T auid, FB_AUN_T aun )
{
    if (auid == 0)
        return;

    int l1_index = AuidL1Tab ::auid_to_l1_index(auid);
    int l2_index = AuidL23Tab::auid_to_l2_index(auid);
    int l3_index = AuidL23Tab::auid_to_l3_index(auid);

    FB_AUN_T l2_table_aun = fh.d->auid_l1.entries[l1_index].get();
    if (l2_table_aun == 0)
    {
        fprintf(stderr, "ERROR: FileBlockLocal :: rename_auid: "
                "no L2 table!\n");
        return;
    }
    FileBlock   * l2fb = get_aun( l2_table_aun );
    AuidL23Tab  * l2tab = (AuidL23Tab*)l2fb->get_ptr();

    FB_AUN_T l3_table_aun = l2tab->entries[l2_index].get();
    if (l3_table_aun == 0)
    {
        fprintf(stderr, "ERROR: FileBlockLocal :: rename_auid: "
                "no L3 table!\n");
        return;
    }
    FileBlock   * l3fb = get_aun( l3_table_aun );
    AuidL23Tab  * l3tab = (AuidL23Tab*)l3fb->get_ptr();

    l3tab->entries[l3_index].set(aun);
    l3fb->mark_dirty();

    release( l2fb );
    release( l3fb );
}

FB_AUN_T
FileBlockLocal :: translate_auid( FB_AUID_T auid )
{
    if (auid == 0)
        return 0;

    int l1_index = AuidL1Tab ::auid_to_l1_index(auid);
    int l2_index = AuidL23Tab::auid_to_l2_index(auid);
    int l3_index = AuidL23Tab::auid_to_l3_index(auid);

    FB_AUN_T l2_table_aun = fh.d->auid_l1.entries[l1_index].get();
    if (l2_table_aun == 0)
        // no translation.
        return 0;
    FileBlock   * l2fb = get_aun( l2_table_aun );
    AuidL23Tab  * l2tab = (AuidL23Tab*)l2fb->get_ptr();

    FB_AUN_T l3_table_aun = l2tab->entries[l2_index].get();
    if (l3_table_aun == 0)
    {
        release( l2fb );
        // no translation
        return 0;
    }
    FileBlock   * l3fb = get_aun( l3_table_aun );
    AuidL23Tab  * l3tab = (AuidL23Tab*)l3fb->get_ptr();

    FB_AUN_T aun = l3tab->entries[l3_index].get();

    release( l2fb );
    release( l3fb );

    return aun;
}

void
FileBlockLocal :: free_auid( FB_AUID_T auid )
{
    if (auid == 0)
        return;

    rename_auid(auid,0);

    uint32_t index = fh.d->info.auid_stack_top.get();
    fh.d->info.auid_stack_top.set(index+1);
    fh.mark_dirty();

    write_stack(index, auid);
}

void
FileBlockLocal :: write_stack( uint32_t index, FB_AUID_T auid )
{
    int sl1_index = AuidL1Tab ::auid_to_l1_index(index);
    int sl2_index = AuidL23Tab::auid_to_l2_index(index);
    int sl3_index = AuidL23Tab::auid_to_l3_index(index);

    FB_AUN_T sl2_table_aun = fh.d->auid_stack_l1.entries[sl1_index].get();
    FileBlock   * sl2fb;
    if (sl2_table_aun == 0)
    {
        // need to allocate a new stack l2 table!

        sl2_table_aun = alloc_aun( sizeof(AuidL23Tab) );
        fh.d->auid_stack_l1.entries[sl1_index].set(sl2_table_aun);
        fh.mark_dirty();
        sl2fb = get_aun( sl2_table_aun, true );
    }
    else
        sl2fb = get_aun( sl2_table_aun );
    AuidL23Tab  * sl2tab = (AuidL23Tab*)sl2fb->get_ptr();

    FB_AUN_T sl3_table_aun = sl2tab->entries[sl2_index].get();
    FileBlock   * sl3fb;
    if (sl3_table_aun == 0)
    {
        // need to alloc a new stack l3 table!

        sl3_table_aun = alloc_aun( sizeof(AuidL23Tab) );
        sl2tab->entries[sl2_index].set( sl3_table_aun );
        sl2fb->mark_dirty();
        sl3fb = get_aun( sl3_table_aun, true );
    }
    else
        sl3fb = get_aun( sl3_table_aun );
    AuidL23Tab  * sl3tab = (AuidL23Tab*)sl3fb->get_ptr();

    sl3tab->entries[sl3_index].set( auid );
    sl3fb->mark_dirty();

    release( sl2fb );
    release( sl3fb );
}

FB_AUID_T
FileBlockLocal :: lookup_stack( uint32_t index )
{
    int sl1_index = AuidL1Tab ::auid_to_l1_index(index);
    int sl2_index = AuidL23Tab::auid_to_l2_index(index);
    int sl3_index = AuidL23Tab::auid_to_l3_index(index);

    FB_AUN_T sl2_table_aun = fh.d->auid_stack_l1.entries[sl1_index].get();
    FileBlock   * sl2fb;
    if (sl2_table_aun == 0)
        // no translation
        return 0;
    sl2fb = get_aun( sl2_table_aun );
    AuidL23Tab  * sl2tab = (AuidL23Tab*)sl2fb->get_ptr();

    FB_AUN_T sl3_table_aun = sl2tab->entries[sl2_index].get();
    FileBlock   * sl3fb;
    if (sl3_table_aun == 0)
    {
        release( sl2fb );
        // no translation
        return 0;
    }
    sl3fb = get_aun( sl3_table_aun );
    AuidL23Tab  * sl3tab = (AuidL23Tab*)sl3fb->get_ptr();

    FB_AUID_T auid = sl3tab->entries[sl3_index].get();

    release( sl2fb );
    release( sl3fb );

    return auid;
}
