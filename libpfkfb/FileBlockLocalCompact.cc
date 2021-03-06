/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/

/** \file FileBlockLocalCompact.cc
 * \brief Implementation of compaction algorithms.
 */


/** \page FileBlockCompacting FileBlock Compacting

The algorithm for compacting a file is as follows:

<ul>
<li>  Unpeeling + Downshifting
  <ul>
  <li> walk AUID and free-stack L1 tables and L2 tables, gathering a
       list of all L2 and L3 table AUNs; also record the location in
       the parent table where its AUN is stored.
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
        <li> start at the beginning of the file, performing
             Downshifting until a free block large enough to contain
             the last block has been coalesced.
        </ul>
     </ul>
  </ul>
</ul>

See also: \ref FileBlockBSTExample

Next: \ref BtreeStructure

*/


/** \todo Compaction algorithm should someday reclaim unused L2/L3 tables. */

//redhat needs this for PRIu64 and friends.
#define __STDC_FORMAT_MACROS 1

#include "FileBlockLocal.h"

#include <stdlib.h>

static bool debug_compaction = false;

enum TableType { AUID, STACK };
enum TableLevel { L2, L3 };

struct L2L3AUN;
class L2L3AUNHashComparator;
typedef DLL3::List <L2L3AUN, 1, false>  L2L3AUNList_t;
typedef DLL3::Hash <L2L3AUN, FB_AUN_T,
                    L2L3AUNHashComparator, 2, false>  L2L3AUNHash_t;

struct L2L3AUN : public L2L3AUNList_t::Links,
                 public L2L3AUNHash_t::Links
{
    TableType type;
    TableLevel level;
    FB_AUN_T aun;
    L2L3AUN * parent;
    int parent_index;
};

class L2L3AUNHashComparator {
public:
    static uint32_t obj2hash  (const L2L3AUN &item) { return item.aun; }
    static uint32_t key2hash  (const FB_AUN_T &key) { return key & 0x7FFFFFFF; }
    static bool     hashMatch (const L2L3AUN &item, const FB_AUN_T &key) {
        return (item.aun == key);
    }
};

class L2L3s {
    L2L3AUNList_t  list;
    L2L3AUNHash_t  hash;
public:
    void add   ( L2L3AUN * item ) {
        list.add_tail(item);
        hash.add(item);
    }
    void remove( L2L3AUN * item ) {
        list.remove(item);
        hash.remove(item);
    }
    L2L3AUN * find ( FB_AUN_T aun ) {
        return hash.find(aun);
    }
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
    if (debug_compaction)
    {
        printf("add table type %d level %d at aun %"
               PRIu64 "; ", ty, lev, aun);
        if (lev == L2)
            printf("  parent is L1 index %d\n", parent_index);
        else
            printf("  parent is L2 at aun %" PRIu64 " index %d\n",
                   l->find(parent_aun)->aun, parent_index);
    }
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
FileBlockLocal :: move_unit( L2L3s *l2l3tables, FB_AUID_T auid,
                             FB_AUN_T aun, FB_AUN_T to_aun )
{
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
            memcpy( &temp, fb->get_ptr(), sizeof(temp) );
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
                    if (debug_compaction)
                    {
                        printf("  moved an L2 AUID table from aun %"
                               PRIu64 " to %" PRIu64 " "
                               "(pi %d)\n",
                               aun, new_aun, l2l3ent->parent_index);
                    }
                }
                else /* type == STACK */
                {
                    fh.d->auid_stack_l1.entries[
                        l2l3ent->parent_index].set(new_aun);
                    if (debug_compaction)
                    {
                        printf("  moved an L2 stack table from aun %"
                               PRIu64 " to %" PRIu64 " (pi %d)\n",
                               aun, new_aun, l2l3ent->parent_index);
                    }
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
                if (debug_compaction)
                {
                    printf("  moved an L3 %s table from aun %"
                           PRIu64 " to %" PRIu64 " (paun %"
                           PRIu64 " pi %d)\n",
                           l2l3ent->type == AUID ? "AUID" : "stack",
                           aun, new_aun, 
                           l2l3ent->parent->aun,
                           l2l3ent->parent_index);
                }
            }

            // must update the aun in this entry, but this also
            // means rehashing it so we can still find it again later.
            l2l3tables->remove(l2l3ent);
            l2l3ent->aun = new_aun;
            l2l3tables->add(l2l3ent);
        }
        else
        {
            printf("  ERROR: can't find an L2/L3 table for aun %"
                   PRIu64 "\n", aun);
            fflush(stdout);
            kill(0,6);
        }
    }
    else
    {
        FB_AUN_T old_aun = 0, new_aun = 0;
        if (debug_compaction)
            old_aun = translate_auid(auid);
        realloc(auid, to_aun, 0);
        if (debug_compaction)
        {
            new_aun = translate_auid(auid);
            printf("  moved auid %d from aun %"
                   PRIu64 " to %" PRIu64 "\n", auid, old_aun, new_aun);
        }
    }
}

//virtual
void
FileBlockLocal :: compact( FileBlockCompactionStatusFunc func, void * arg )
{
    L2L3s  l2l3tables;
    L2L3AUN * l2l3ent;
    int i;
    FB_AUN_T next_ds_au;

    debug_compaction = (getenv("DEBUG_COMPACTION") != NULL);

    // the only block that can be in use is the FileHeader

    if (active_blocks.get_cnt() > 1)
    {
        fprintf(stderr, "ERROR: FileBlockLocal :: compact: "
                "there are %d blocks still in use!\n",
                active_blocks.get_cnt());
        return;
    }

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
        FB_AUN_T aun, last_aun;
        uint32_t free_aus, num_aus;
        FB_AUID_T auid;
        int bucket;

        last_aun = fh.d->info.num_aus.get();        
        free_aus = fh.d->info.free_aus.get();

        if (debug_compaction)
        {
            printf("free_aus = %d, file_size = %" PRIu64 ", percent = %u\n",
                   free_aus, last_aun, (uint32_t)((free_aus*100)/last_aun));
        }

        {
            FileBlockStats stats;

            stats.au_size      = AU_SIZE;
            stats.used_aus     = fh.d->info.used_aus    .get();
            stats.free_aus     = fh.d->info.free_aus    .get();
            stats.used_regions = fh.d->info.used_extents.get();
            stats.free_regions = fh.d->info.free_extents.get();
            stats.num_aus      = fh.d->info.num_aus     .get();
            
            if (func(&stats, arg) == false)
                break;
        }

        AUHead  au(bc);
        au.get(last_aun);
        aun = au.d->prev.get();
        au.get(aun);
        num_aus = au.d->size();
        auid = au.d->auid();
        au.release();
        if (debug_compaction)
        {
            printf("last used au is at %" PRIu64 ", auid %d,  size is %d\n",
                   aun, auid, num_aus);
        }

        // don't bother downshifting the whole file just for one block.
        if (num_aus > free_aus)
            break;

        bucket = ffu_bucket(num_aus);
        if (debug_compaction)
        {
            printf("bucket is %d\n", bucket);
        }
        if (bucket == (BucketList::NUM_BUCKETS-1))
        {
            if (debug_compaction)
            {
                printf("cannot move this piece, must start downshifting\n");
            }
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
                if (debug_compaction)
                {
                    printf("found a free region of size %d at %" PRIu64 "\n",
                           au.d->size(), next_ds_au);
                }
                if (au.d->size() == 0)
                {
                    // have we downshifted the entire file?
                    // a free region of size 0 can only be 
                    // the end-marker.
                    break;
                }

                if (au.d->size() >= num_aus_desired)
                {
                    if (debug_compaction)
                    {
                        printf("which is large enough for piece we "
                               "want to move\n");
                    }
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
                if (debug_compaction)
                {
                    printf("downshifting auid %d size %d from aun %"
                           PRIu64 " to %" PRIu64 ":\n",
                           auid, num_aus, naun, next_ds_au );
                }
                move_unit( &l2l3tables, auid, naun, next_ds_au );
                if (debug_compaction)
                {
                    naun = next_ds_au + num_aus;
                    au.get(naun);
                    printf("after moving, next %s region size at aun %"
                           PRIu64 " is %d\n",
                           au.d->used() ? "USED" : "FREE",
                           naun, au.d->size());
                    au.release();
                }
            }
        }
        else
        {
            move_unit( &l2l3tables, auid, aun, 0 );
        }
    }

done:
    while ((l2l3ent = l2l3tables.dequeue_head()) != NULL)
        delete l2l3ent;

    off_t pos;

    pos = (off_t)(fh.d->info.num_aus.get() + 1) * AU_SIZE;

    bc->truncate( pos );
}
