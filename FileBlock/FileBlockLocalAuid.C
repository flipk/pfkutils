
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

/** \file FileBlockLocalAuid.C
 * \brief All methods for AUID management: allocation, free, stack, and
 *     L1/L2/L3 table maintanence.
 * \author Phillip F Knaack
 */

#include "FileBlockLocal.H"

#include <stdlib.h>

FB_AUID_T
FileBlockLocal :: alloc_auid( FB_AUN_T aun )
{
    FB_AUID_T auid = 0;
    UINT32 stack_top;

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
        // no translation
        return 0;
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

    UINT32 index = fh.d->info.auid_stack_top.get();
    fh.d->info.auid_stack_top.set(index+1);
    fh.mark_dirty();

    write_stack(index, auid);
}

void
FileBlockLocal :: write_stack( UINT32 index, FB_AUID_T auid )
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
FileBlockLocal :: lookup_stack( UINT32 index )
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
        // no translation
        return 0;
    sl3fb = get_aun( sl3_table_aun );
    AuidL23Tab  * sl3tab = (AuidL23Tab*)sl3fb->get_ptr();

    FB_AUID_T auid = sl3tab->entries[sl3_index].get();

    release( sl2fb );
    release( sl3fb );

    return auid;
}
