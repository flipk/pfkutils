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

The algorithm for compacting a file is as follows:

<ul>
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

#define DEBUG_COMPACTION 0

enum TableType { AUID, STACK };
enum TableLevel { L2, L3 };

enum {
    DLL2_INDEX_LIST,
    DLL2_INDEX_HASH,
    DLL2_NUM_LISTS
};

struct L2L3AUN {
    LListLinks <L2L3AUN> links[DLL2_NUM_LISTS];
    TableType type;
    TableLevel level;
    FB_AUN_T aun;
    L2L3AUN * parent;
    int parent_index;
};

class L2L3AUNHashComparator {
public:
    static int hash_key( L2L3AUN * item ) { return item->aun; }
    static int hash_key( FB_AUN_T key ) { return key & 0x7FFFFFFF; }
    static bool hash_key_compare( L2L3AUN * item, FB_AUN_T key ) {
        return (item->aun == key);
    }
};

typedef LList <L2L3AUN,DLL2_INDEX_LIST> L2L3AUNList;
typedef LListHash <L2L3AUN,FB_AUN_T,
                   L2L3AUNHashComparator,DLL2_INDEX_HASH> L2L3AUNHash;

class L2L3s {
    L2L3AUNList  list;
    L2L3AUNHash  hash;
public:
    void add   ( L2L3AUN * item ) { list.add   (item); hash.add   (item); }
    void remove( L2L3AUN * item ) { list.remove(item); hash.remove(item); }
    L2L3AUN * find ( FB_AUN_T aun ) { return hash.find(aun); }
    L2L3AUN * dequeue_head( void ) {
        L2L3AUN * ret = list.dequeue_head();
        if (ret)
            hash.remove(ret);
        return ret;
    }
};

/** add an entry to the L2L3s list.
 * \param l   the list to modify
 * \param aun   the AUN of the L2/L3 table to add to the list.
 * \param ty    type of the table being added, AUID or STACK
 * \param lev   the table level, L2 or L3
 * \param parent_aun  the AUN of the parent table (ignored if L2)
 * \param parent_index  the index in the parent table to modify
 */
static void
addent( L2L3s * l, FB_AUN_T aun,
        TableType ty, TableLevel lev,
        FB_AUN_T parent_aun, int parent_index )
{
    L2L3AUN * ent = new L2L3AUN;
    ent->type = ty;
    ent->level = lev;
    ent->aun = aun;
    ent->parent = l->find(parent_aun);
    ent->parent_index = parent_index;
    l->add(ent);
#if DEBUG_COMPACTION
    printf("add table type %d level %d at aun %d; ", ty, lev, aun);
    if (lev == L2)
        printf("  parent is L1 index %d\n", parent_index);
    else
        printf("  parent is L2 at %d index %d\n",
               l->find(parent_aun)->aun, parent_index);
#endif
}

void
FileBlockLocal :: walkl2( void /*L2L3s*/ * l,
                          FB_AUN_T tabaun, int /*TableType*/ ty )
{
    FileBlock   * l2fb = get_aun( tabaun );
    AuidL23Tab  * l2tab = (AuidL23Tab*)l2fb->get_ptr();
    int i;
    FB_AUN_T aun;

    for (i=0; i < AuidL23Tab::L23_ENTRIES; i++)
    {
        aun = l2tab->entries[i].get();
        if (aun != 0)
            addent( (L2L3s*) l, aun,
                    (TableType)ty, L3,
                    tabaun, i );
    }

    release( l2fb );
}

void
FileBlockLocal :: move_unit( void *l, FB_AUID_T auid,
                             FB_AUN_T aun, FB_AUN_T to_aun,
                             int num_aus )
{
    L2L3s * l2l3tables = (L2L3s*) l;
    L2L3AUN * l2l3ent;

    if (auid == 0)
    {
        l2l3ent = l2l3tables->find(aun);
        if (l2l3ent)
        {
            AuidL23Tab temp;
            FileBlock * fb;
            FB_AUN_T new_aun;

            fb = get_aun(aun);
            memcpy( &temp, fb->get_ptr(), fb->get_size() );
            release(fb);
            free_aun(aun);
            if (to_aun == 0)
                new_aun = alloc_aun( sizeof(temp) );
            else
            {
                AUHead junk_au(bc);
                new_aun = alloc_aun(to_aun, &junk_au, sizeof(temp) );
            }
            fb = get_aun(new_aun,true);
            memcpy(fb->get_ptr(), &temp, sizeof(temp) );
            fb->mark_dirty();
            release(fb);

            // update parent's entry
            if (l2l3ent->level == L2)
            {
                // moved an L2, so must update L1 table
                if (l2l3ent->type == AUID)
                {
                    fh.d->auid_l1.entries[
                        l2l3ent->parent_index].set(new_aun);
#if DEBUG_COMPACTION
                    printf("  moved an L2 AUID table from %d to %d (pi %d)\n",
                           aun, new_aun, l2l3ent->parent_index);
#endif
                }
                else /* type == STACK */
                {
                    fh.d->auid_stack_l1.entries[
                        l2l3ent->parent_index].set(new_aun);
#if DEBUG_COMPACTION
                    printf("  moved an L2 stack table from %d to %d (pi %d)\n",
                           aun, new_aun, l2l3ent->parent_index);
#endif
                }
                fh.mark_dirty();
            }
            else
            {
                // we moved an L3 table, so we're updating an L2 table
                fb = get_aun(l2l3ent->parent->aun);
                AuidL23Tab * t = (AuidL23Tab *)fb->get_ptr();
                t->entries[l2l3ent->parent_index].set( new_aun );
                fb->mark_dirty();
                release(fb);
#if DEBUG_COMPACTION
                printf("  moved an L3 %s table from %d to %d (paun %d pi %d)\n",
                       l2l3ent->type == AUID ? "AUID" : "stack",
                       aun, new_aun, 
                       l2l3ent->parent->aun,
                       l2l3ent->parent_index);
#endif
            }

            // must update the aun in this entry, but this also
            // means rehashing it so we can still find it again later.
            l2l3tables->remove(l2l3ent);
            l2l3ent->aun = new_aun;
            l2l3tables->add(l2l3ent);
        }
        else
        {
            printf("  ERROR: can't find an L2/L3 table\n");
        }
    }
    else
    {
#if DEBUG_COMPACTION
        FB_AUN_T old_aun, new_aun;
        old_aun = translate_auid(auid);
#endif
        realloc(auid, to_aun, 0);
#if DEBUG_COMPACTION
        new_aun = translate_auid(auid);
        printf("  moved auid %d from %d to %d\n", auid, old_aun, new_aun);
#endif
    }
}

//virtual
void
FileBlockLocal :: compact( bool full )
{
    L2L3s  l2l3tables;
    L2L3AUN * l2l3ent;
    int i;
    FB_AUN_T next_ds_au;

    // build the list of L2/L3 tables.

    for (i=0; i < AuidL1Tab::L1_ENTRIES; i++)
    {
        FB_AUN_T aun;
        aun = fh.d->auid_l1.entries[i].get();
        if (aun != 0)
        {
            addent(&l2l3tables, aun, AUID, L2, 0, i);
            walkl2(&l2l3tables, aun, AUID);
        }
        aun = fh.d->auid_stack_l1.entries[i].get();
        if (aun != 0)
        {
            addent(&l2l3tables, aun, STACK, L2, 0, i);
            walkl2(&l2l3tables, aun, STACK);
        }
    }

    next_ds_au = fh.d->info.first_au.get();

    while (1)
    {
        FB_AUN_T aun, last_aun, free_aus;
        FB_AUID_T auid;
        int num_aus, bucket;

        last_aun = fh.d->info.num_aus.get();        
        free_aus = fh.d->info.free_aus.get();

#if DEBUG_COMPACTION
        printf("free_aus = %d, file_size = %d, percent = %d\n",
               free_aus, last_aun, (free_aus*100)/last_aun);
#endif

        if (((free_aus*100)/last_aun) < 2)
            break;

        AUHead  au(bc);
        au.get(last_aun);
        aun = au.d->prev.get();
        au.get(aun);
        num_aus = au.d->size();
        auid = au.d->auid();
        au.release();
#if DEBUG_COMPACTION
        printf("last used au is at %d, auid %d,  size is %d\n",
               aun, auid, num_aus);
#endif
        bucket = ffu_bucket(num_aus);
#if DEBUG_COMPACTION
        printf("bucket is %d\n", bucket);
#endif
        if (bucket == (BucketList::NUM_BUCKETS-1))
        {
#if DEBUG_COMPACTION
            printf("cannot move this piece, must start downshifting\n");
#endif
            int num_aus_desired = num_aus;

            while (1)
            {
                // find a free region.
                while (1)
                {
                    if (!au.get(next_ds_au))
                        goto done;
                    if (!au.d->used())
                        break;
                    next_ds_au += au.d->size();
                }
#if DEBUG_COMPACTION
                printf("found a free region of size %d at %d\n",
                       au.d->size(), next_ds_au);
#endif
                if (au.d->size() >= num_aus_desired)
                {
#if DEBUG_COMPACTION
                    printf("which is large enough for piece we want to move\n");
#endif
                    break;
                }

                // the next region should be used, because the file
                // should never have two free regions adjacent.
                // move it down.

                FB_AUN_T naun;
                naun = next_ds_au + au.d->size();
                if (!au.get(naun))
                    goto done;
                num_aus = au.d->size();
                auid = au.d->auid();
                au.release();
#if DEBUG_COMPACTION
                printf("downshifting auid %d size %d from %d to %d:\n",
                       auid, num_aus, naun, next_ds_au );
#endif
                move_unit( &l2l3tables, auid, naun, next_ds_au, num_aus );
#if DEBUG_COMPACTION
                naun = next_ds_au + num_aus;
                au.get(naun);
                printf("after moving, next %s region size at %d is %d\n",
                       au.d->used() ? "USED" : "FREE",
                       naun, au.d->size());
                au.release();
#endif
            }
        }
        else
        {
            move_unit( &l2l3tables, auid, aun, 0, num_aus );
        }
    }

done:
    while ((l2l3ent = l2l3tables.dequeue_head()) != NULL)
        delete l2l3ent;
}
