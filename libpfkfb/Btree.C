
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

/** \file Btree.C
 * \brief implementation of Btree and BtreeInternal objects
 * \author Phillip F Knaack
 */

/** \page BtreeStructure BTREE File Interface

The BTREE is a data storage/retrieval mechanism.  It uses a "key" data
structure to index the data, so that if the proper key is provided,
the corresponding data can be located quickly.

At the lowest conceptual level, a "key" and its corresponding "data"
are just sequences of bytes of arbitrary length and contents.

Keys must be unique.  If a "put" is done on two key/data pairs where
the keys are the same number of bytes and have the same contents, the
data portion of the second "put" will replace the data of the first in
the file.

The "key" and "data" units are specified with UCHAR pointers and an int
specifying a length.  A UCHAR and int together are known as a "datum".

To retrieve data from a Btree, construct a key datum with the proper
contents, and call the "get" method.  It will return a data datum which
has been populated with the matching data.
 
\todo document btree

\section BtreeTemplate BTREE Template types

Next: \ref BtreeInternalStructure

 */

/** \page BtreeInternalStructure BTREE Internal Structure

\todo document btree internals

\section BtreeNodeOnDisk Btree node, on-disk layout

The BTREE node, on disk, has the following layout:

<ul>
<li> UINT32_t magic
     <ul>
     <li> which must be set to _BTNodeDisk::MAGIC
     </ul>
<li> UINT16_t numitems
     <ul>
     <li> the bottom 14 bits (13-0) of this value encodes the number of items
     	  stored in this node.  this can be no greator than order-1, and
	  must be greator than or equal to order/2 (except in the root node,
	  where the number of items can be anywhere from 0 to order-1).
     <li> bit 14 indicates if this is a root node.  obviously this can be
     	  determined by the fact that the BTInfo points to this node, but
	  it is an extra sanity check.
     <li> bit 15 indicates if this is a leaf node.  in this case the pointers
     	  should not be followed, and inserts should take place here.
     </ul>
<li> BTNodeItem items[order-1]:
     <ul>
     <li> FB_AUID_t ptr
     	  <ul>
	  <li> this is a FileBlock ID number of a pointer to a child node.
	  </ul>
     <li> UINT16_t keystart
     <li> UINT16_t keysize
     	  <ul>
	  <li> these indicate where the key data for this item begin within
	       the keydata element below.
     	  </ul>
     <li> FB_AUID_t data
     	  <ul>
	  <li> this is a FileBlock ID number of the data corresponding to
	       the key above.
	  </ul>
     </ul>
<li> FB_AUID_t ptr
     <ul>
     <li> the number of ptrs in a node is \em order, however the number
     	  of key/data items in a node is \em order-1.  so there is an
	  extra ptr at the end of the array to round out the node.
     <li> since \em ptr is the first element of the items array, it thus
     	  works to access items[order].ptr (even though the dimension of the
	  array is order-1), to access this extra ptr.
     </ul>
<li> UCHAR keydata[]
     <ul>
     <li> this is a variable-length array of data where the key data for
     	  all the items begins.  the \em keystart and \em keysize elements
	  indicate offsets and sizes in this array.  all of the key data
	  are contiguous in this array, thus the total size of this
	  array is items[order-2].keystart + items[order-1].keysize.
     </ul>
</ul>

Next: \ref Templates

*/

#include "Btree.H"
#include "Btree_internal.H"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>

const char BtreeInternal::BTInfoFileInfoName[] = "PFKBTREEINFO";

//static
Btree *
Btree :: open( FileBlockInterface * _fbi )
{
    if (BtreeInternal::valid_file(_fbi))
        return new BtreeInternal(_fbi);
    return NULL;
}

//static
bool
Btree :: init_file( FileBlockInterface * _fbi, int order )
{
    FB_AUID_T  root_fbn;

    if (order < 0 || order > BtreeInternal::MAX_ORDER || 
        (order & 1) == 0)
        return false;

    BTNode n(_fbi, order, 0);
    root_fbn = n.get_fbn();
    n.store();

    FB_AUID_T  info_fbn;
    BTInfo  info(_fbi);

    info_fbn = info.alloc();
    info.get(info_fbn, true);
    info.d->magic.set ( _BTInfo::MAGIC );
    info.d->bti_fbn.set( info_fbn );
    info.d->root_fbn.set( root_fbn );
    info.d->numnodes.set( 1 );
    info.d->numrecords.set( 0 );
    info.d->depth.set( 1 );
    info.d->order.set( order );

    _fbi->set_data_info_block( info_fbn,
                               (char*)BtreeInternal::BTInfoFileInfoName );

    return true;
}

//static
bool
Btree :: valid_file( FileBlockInterface * fbi )
{
    return BtreeInternal::valid_file(fbi);
}

//static
bool
BtreeInternal :: valid_file( FileBlockInterface * _fbi )
{
    FB_AUID_T info_fbn;
    BTInfo info(_fbi);

    info_fbn = _fbi->get_data_info_block( (char*) BTInfoFileInfoName );

    if (!info_fbn)
        return false;

    info.get(info_fbn);
    if (info.d->magic.get() != _BTInfo::MAGIC)
        return false;

    int order = info.d->order.get();

    if (order < 0 || order > MAX_ORDER || (order & 1) == 0)
        return false;

    if (info.d->bti_fbn.get() != info_fbn)
        return false;

    return true;
}

//static
Btree *
Btree :: _openFile( const char * filename, int max_bytes,
                    bool create, int mode, int order )
{
    FileBlockInterface * fbi = NULL;
    fbi = FileBlockInterface::_openFile(filename,max_bytes,create,mode);
    if (!fbi)
        return NULL;
    if (create)
        Btree::init_file( fbi, order );
    Btree * bt = open(fbi);
    if (bt)
        return bt;
    delete fbi;
    return NULL;
}

BtreeInternal :: BtreeInternal( FileBlockInterface * _fbi )
    : info(_fbi)
{
    fbi = _fbi;
    info_fbn = _fbi->get_data_info_block( (char*) BTInfoFileInfoName );
    if (!info_fbn)
    {
    error:
        fprintf(stderr, "error: not a btree file\n");
        exit(1);
    }
    info.get(info_fbn);
    if (info.d->magic.get() != _BTInfo::MAGIC)
        goto error;
    BTREE_ORDER = info.d->order.get();
    if (BTREE_ORDER < 0 || BTREE_ORDER > MAX_ORDER || (BTREE_ORDER & 1) == 0)
        goto error;
    HALF_ORDER = BTREE_ORDER / 2;
    ORDER_MO = BTREE_ORDER - 1;
    if (info.d->bti_fbn.get() != info_fbn)
        goto error;
    node_size = _BTNodeDisk::node_size(BTREE_ORDER);
    node_cache = new BTNodeCache( fbi, BTREE_ORDER, MAX_NODES );
    iterate_inprogress = false;
}

//virtual
BtreeInternal :: ~BtreeInternal(void)
{
    delete node_cache;
    info.release();
    delete fbi;
}

//static
int
BtreeInternal :: compare_keys( UCHAR * key, int keylen, BTKey * two )
{
    int onesize = keylen;
    int twosize = two->keylen;
    int comparelen = (onesize > twosize) ? twosize : onesize;
    int result;

    result = memcmp( key, two->data, comparelen );
    if (result != 0)
        return (result > 0) ? 1 : -1;
    if (onesize > twosize)
        return 1;
    else if (onesize < twosize)
        return -1;
    return 0;
}

int
BtreeInternal :: walknode( BTNode * n, UCHAR * key, int keylen, bool *exact )
{
    if (n->numitems == 0)
    {
        *exact = false;
        return 0;
    }

    int i,res = 0;
    for (i=0; i < n->numitems; i++)
    {
        res = compare_keys( key, keylen, n->keys[i] );
        if (res <= 0)
            break;
    }

    *exact = (res == 0);

    return i;
}

FB_AUID_T
BtreeInternal :: splitnode( BTNode * n, BTKey ** key, FB_AUID_T * data_fbn,
                            FB_AUID_T rightptr, int idx )
{
    int i;
    enum { R_PIVOT, R_LEFT, R_RIGHT } where = R_PIVOT;

    if (idx < HALF_ORDER)
        // the key belongs in the left node; the pivot record for the two
        // nodes (to be promoted to the parent node) will be in the middle
        // of the old node.
        where = R_LEFT;
    else if (idx == HALF_ORDER)
        // the provided key IS the pivot record, and will itself be promoted
        // to the parent node.
        where = R_PIVOT;
    else
        // the provided key belongs in the right node; the pivot record
        // to be promoted exists in the middle of the old node.
        where = R_RIGHT;

    // nr short for 'new right'
    BTNode * nr = node_cache->new_node();
    info.d->numnodes.set( info.d->numnodes.get() + 1 );

    nr->leaf = n->leaf;
    nr->root = false;
    n->numitems = HALF_ORDER;
    nr->numitems = HALF_ORDER;
    n->mark_dirty();

    switch (where)
    {
    case R_PIVOT:
        // the new node is the promoted pivot, so don't update
        // key or data_fbn; just move half of n's items to nr
        // and we're done.  the first left-ptr of the new right
        // is the passed-in rightptr.

        nr->ptrs[0] = rightptr;

        for (i=0; i < HALF_ORDER; i++)
        {
            nr->keys[i] = n->keys[i + HALF_ORDER];
            nr->datas[i] = n->datas[i + HALF_ORDER];
            nr->ptrs[i + 1] = n->ptrs[i + HALF_ORDER + 1];

            n->keys[i + HALF_ORDER] = NULL;
            n->datas[i + HALF_ORDER] = 0;
            n->ptrs[i + HALF_ORDER + 1] = 0;
        }
        break;

    case R_LEFT:
        // copy right half of left node into right node

        for (i=0; i < HALF_ORDER; i++)
        {
            nr->keys[i] = n->keys[i + HALF_ORDER];
            nr->datas[i] = n->datas[i + HALF_ORDER];
            nr->ptrs[i] = n->ptrs[i + HALF_ORDER];

            n->keys[i + HALF_ORDER] = NULL;
            n->datas[i + HALF_ORDER] = 0;
            n->ptrs[i + HALF_ORDER] = 0;
        }
        // pick up one trailing ptr
        nr->ptrs[i] = n->ptrs[i + HALF_ORDER];

        // now slide over remaining components of left
        // node that must move to accomodate new item.

        for (i = (HALF_ORDER-1); i >= idx; i--)
        {
            n->keys[i+1] = n->keys[i];
            n->datas[i+1] = n->datas[i];
            n->ptrs[i+2] = n->ptrs[i+1];
        }

        // insert new item.

        n->keys[idx] = *key;
        n->datas[idx] = *data_fbn;
        n->ptrs[idx+1] = rightptr;

        // pivot record to promote is left in leftnode.
        break;

    case R_RIGHT:
        // start copying things over to the right;
        // when we come to the place where the pivot is
        // supposed to go, insert it.

        idx -= HALF_ORDER+1;
        int j = HALF_ORDER+1;

        nr->ptrs[0] = n->ptrs[j];
        n->ptrs[j] = 0;

        for (i=0; i < HALF_ORDER; i++)
        {
            if (i == idx)
            {
                // this is where pivot rec goes.
                nr->keys[i] = *key;
                nr->datas[i] = *data_fbn;
                nr->ptrs[i+1] = rightptr;
            }
            else
            {
                nr->keys[i] = n->keys[j];
                nr->datas[i] = n->datas[j];
                nr->ptrs[i+1] = n->ptrs[j+1];
                n->keys[j] = NULL;
                n->datas[j] = 0;
                n->ptrs[j+1] = 0;
                j++;
            }
        }

        // pivot record to promote is left in the leftnode.
        break;
    }

    if (where != R_PIVOT)
    {
        // highest remaining element of leftnode becomes
        // the new promoted pivot record.
        *key = n->keys[HALF_ORDER];
        *data_fbn = n->datas[HALF_ORDER];
        n->keys[HALF_ORDER] = NULL;
        n->datas[HALF_ORDER] = 0;
    }

    FB_AUID_T nr_fbn = nr->get_fbn();
    node_cache->release(nr);
    return nr_fbn;
}

//virtual
bool
BtreeInternal :: get( UCHAR * key, int keylen, FB_AUID_T * data )
{
    bool ret = false;
    bool exact = false;
    int idx;
    FB_AUID_T curfbn;
    BTNode * curn;
    FB_AUID_T data_fbn = 0;

    curfbn = info.d->root_fbn.get();

    while (!exact && curfbn != 0)
    {
        curn = node_cache->get(curfbn);
        exact = false;
        idx = walknode( curn, key, keylen, &exact );
        if (exact)
            data_fbn = curn->datas[idx];
        if (curn->leaf)
            curfbn = 0;
        else
            curfbn = curn->ptrs[idx];
        node_cache->release(curn);
    }

    if (exact)
    {
        *data = data_fbn;
        ret = true;
    }

    return ret;
}

struct nodewalker {
    struct nodewalker * next;
    FB_AUID_T fbn;
    BTNode * node;
    int idx;
    //
    nodewalker(struct nodewalker * _nxt, FB_AUID_T _fbn,
               BTNode * _n, int _idx) {
        next = _nxt; fbn = _fbn; node = _n; idx = _idx;
    }
};

//virtual
bool
BtreeInternal :: put( UCHAR * key, int keylen, FB_AUID_T data_id,
                      bool replace, bool * replaced, FB_AUID_T * old_data_id )
{
    if (iterate_inprogress)
    {
        fprintf(stderr, "ERROR: cannot modify database while iterate "
                "is in progress!\n");
        exit(1);
    }

    if (replace)
    {
        if (replaced == NULL || old_data_id == NULL)
        {
            fprintf(stderr, "ERROR: Btree::put: must provide "
                    "replaced/old data ptrs\n");
            exit(1);
        }
        *replaced = false;
    }

    bool ret = false;
    nodewalker * nodes = NULL, * nw = NULL;
    FB_AUID_T curfbn, right_fbn = 0;
    BTNode * curn;
    int curidx;
    BTKey * newkey = NULL;

    // start walking down from the rootnode a level at a time,
    // until we either find an exact match or we hit a leaf.
    // each time we go down a level, add an element to the nodstor
    // linked-list (head-first) so that we can trace our path
    // back up the tree if we need to split and promote.

    curfbn = info.d->root_fbn.get();
    curn = node_cache->get( curfbn );

    while(1)
    {
        bool exact;
        curidx = walknode( curn, key, keylen, &exact );
        nodes = new nodewalker(nodes, curfbn, curn, curidx);

        if (exact)
        {
            // found an exact match in the tree.
            // if the new data block id is different from the one we found,
            // free the old one and insert the new one to the node.

            if (replace == false)
                goto out;

            if (curn->datas[curidx] != data_id)
            {
                *replaced = true;
                *old_data_id = curn->datas[curidx];
                curn->datas[curidx] = data_id;
                curn->mark_dirty();
            }

            ret = true;
            goto out;
        }

        // break out if we're at the leaf node, or
        // proceed to the next level down the tree.

        if (curn->leaf)
            break;

        curfbn = curn->ptrs[curidx];
        curn = node_cache->get( curfbn );
    }

    // if we hit this point that means we haven't seen an
    // exact match, so we should insert at the leaf level.
    // we need to make a new BTKey.

    newkey = new(keylen) BTKey(keylen);
    memcpy(newkey->data, key, keylen);

    // if we overflow a node here, split the node and start
    // promoting up the tree. keep splitting and promoting up
    // until we reach a node where we're not overflowing a node.
    // if we reach the root and we overflow the root, split the
    // root and create a new root with the promoted item.

    ret = true;
    info.d->numrecords.set( info.d->numrecords.get() + 1 );
    info.mark_dirty();

    nw = nodes;
    nodes = nw->next;
    curfbn = nw->fbn;
    curn = nw->node;
    curidx = nw->idx;
    delete nw;

    while(1)
    {
        if (curn->numitems < ORDER_MO)
        {
            int i;

            // this node is not full, so we can insert
            // into this node at the appropriate index,
            // and be done. slide over recs in this node
            // to make room for the new rec.

            curn->numitems ++;
            for (i = ORDER_MO; i > curidx; i--)
                curn->ptrs[i] = curn->ptrs[i-1];

            for (i = ORDER_MO-1; i >= curidx; i--)
            {
                curn->keys[i] = curn->keys[i-1];
                curn->datas[i] = curn->datas[i-1];
            }

            curn->keys[curidx] = newkey;
            curn->datas[curidx] = data_id;
            curn->ptrs[curidx+1] = right_fbn;
            curn->mark_dirty();

            node_cache->release(curn);

            break;
        }

        // this node is full, so split it and prepare to 
        // promote a pivot record. split_node locates the pivot
        // and updates 'newr' to point to the pivot.

        right_fbn = splitnode( curn, &newkey, &data_id, right_fbn, curidx );

        nw = nodes;
        if (nw)
        {
            node_cache->release(curn);
            nodes = nw->next;
            curfbn = nw->fbn;
            curn = nw->node;
            curidx = nw->idx;
            delete nw;
        }
        else
        {
            // we're at the root node, so we must create
            // a new root node and insert the pivot record
            // by itself here.

            BTNode * newroot = node_cache->new_node();
            newroot->numitems = 1;
            newroot->ptrs[0] = info.d->root_fbn.get();
            newroot->ptrs[1] = right_fbn;
            newroot->keys[0] = newkey;
            newroot->datas[0] = data_id;
            curn->root = false;
            newroot->root = true;
            newroot->leaf = false;
            info.d->root_fbn.set(newroot->get_fbn());
            info.d->numnodes.set( info.d->numnodes.get() + 1 );
            info.d->depth.set( info.d->depth.get() + 1 );
            node_cache->release(curn);
            node_cache->release(newroot);
            break;
        }
    }

out:
    while(nodes)
    {
        nw = nodes;
        nodes = nw->next;
        node_cache->release(nw->node);
        delete nw;
    }

    if (ret == false)
        if (newkey)
            delete newkey;
    return ret;
}

//virtual
bool
BtreeInternal :: del( UCHAR * key, int keylen, FB_AUID_T *old_data_id )
{
    bool ret = false;

    if (old_data_id == NULL)
    {
        fprintf(stderr, "ERROR: Btree::del: must provide old data ptr\n");
        exit(1);
    }

    if (iterate_inprogress)
    {
        fprintf(stderr, "ERROR: cannot modify database while iterate "
                "is in progress!\n");
        exit(1);
    }

    nodewalker * nodes = NULL, * nw = NULL;
    bool exact;
    int idx;
    FB_AUID_T curfbn;
    BTNode * curn;

    curfbn = info.d->root_fbn.get();
    curn = node_cache->get(curfbn);

    // begin walking down the tree looking for the
    // item. at each node, store a pointer and index
    // to the node in the nodstore.
    // if we find an exact match, bail out even if its
    // not the leaf.

    while(1)
    {
        idx = walknode( curn, key, keylen, &exact );
        nodes = new nodewalker(nodes, curfbn, curn, idx);
        if (exact || curn->leaf)
            break;
        curfbn = curn->ptrs[idx];
        curn = node_cache->get(curfbn);
    }

    // if we didn't find an exact match, unlock all nodes
    // in the nodstore and return failure code.

    if (!exact)
    {
        while(nodes)
        {
            nw = nodes;
            nodes = nw->next;
            node_cache->release(nw->node);
            delete nw;
        }
        return false;
    }

    // go to the right on this last node.
    nodes->idx ++;

    // record which node contains the deleted item.
    BTNode * foundn = curn;
    int foundnindex = idx;

    // free the item.
    *old_data_id = curn->datas[idx];
    delete curn->keys[idx];
    curn->keys[idx] = NULL;

    // if we're not at the leaf, continue walking down to the 
    // leaf, and store the path we used to get there. the first
    // time, go to the right, from then on go to leftmost.
    // this way we can end up at the smallest node to the right
    // of the target (for pulling up).

    bool _first = true;
    while ( !curn->leaf )
    {
        int fetchwhich = 0;
        if ( _first )
        {
            _first = false;
            fetchwhich = idx + 1;
        }
        curfbn = curn->ptrs[fetchwhich];
        curn = node_cache->get(curfbn);
        nodes = new nodewalker(nodes, curfbn, curn, 0);
        idx = 0;
    }

    // if we've walked down to a leaf where the deleted item
    // was in a nonleaf, pull up the smallest leaf item into
    // the deleted item's slot.

    if (foundn != curn)
    {
        foundn->keys[foundnindex] = curn->keys[idx];
        foundn->datas[foundnindex] = curn->datas[idx];
        foundn->mark_dirty();
        curn->mark_dirty();
    }

    // now in the leaf, slide over the remaining items 
    // to take up the space of the item that was 
    // removed or deleted.
    // don't need to mess with ptrs because we know
    // that this is a leaf.

    int i;
    for ( i = idx; i < ORDER_MO; i++ )
    {
        curn->keys[i] = curn->keys[i+1];
        curn->datas[i] = curn->datas[i+1];
    }

    // clear out the right-hand slot left behind by the slide.

    curn->keys[ORDER_MO-1] = NULL;
    curn->datas[ORDER_MO-1] = 0;
    curn->numitems --;
    curn->mark_dirty();

    ret = true;

    bool oldrootfree = false;
    FB_AUID_T oldrootfbn = 0;

    // start analyzing nodes for redistribution.
    // return back up the list of nodes until we 
    // no longer need to steal or coalesce.

    nw = nodes;
    while(1)
    {
        BTNode * parent;
        int parentidx;
        BTNode * lsib = NULL; // left sibling
        BTNode * rsib = NULL; // right sibling
        FB_AUID_T lfbn = 0, rfbn = 0; // left and right block ids
        enum sibwho { SIB_NONE, SIB_LEFT, SIB_RIGHT };
        sibwho whichsib_steal = SIB_NONE;
        sibwho whichsib_coalesce = SIB_NONE;

        curn = nw->node;
        curfbn = nw->fbn;

        if (curn->numitems >= HALF_ORDER  ||  curn->root)
            break;

        nw = nw->next;
        if (!nw)
        {
            fprintf(stderr,"Btree::delete: case that should never happen!\n");
            break;
        }

        parent = nw->node;
        parentidx = nw->idx;

        // check left sib -- if more than half full we can use 
        // left sib's rightmost entry to steal; if its only
        // exactly half full, mark it as candidate for coalescing.

        // first check that we actually have a left sib.
        if (parentidx > 0)
        {
            lfbn = parent->ptrs[parentidx-1];
            lsib = node_cache->get(lfbn);
            if (lsib->numitems > HALF_ORDER)
                whichsib_steal = SIB_LEFT;
            else
                whichsib_coalesce = SIB_LEFT;
        }

        // check right sib -- if more than half full we can use
        // right sib's leftmost entry to steal. if its only half
        // full, mark it as candidate for coalescing.

        // if we can steal from the left, don't bother
        // looking to the right. then check that we
        // actually have a right sib.
        if (whichsib_steal == SIB_NONE   &&
            parentidx < parent->numitems)
        {
            rfbn = parent->ptrs[parentidx+1];
            rsib = node_cache->get(rfbn);
            if (rsib->numitems > HALF_ORDER)
                whichsib_steal = SIB_RIGHT;
            else
                whichsib_coalesce = SIB_RIGHT;
        }

        if ( whichsib_steal == SIB_NONE && 
             whichsib_coalesce == SIB_NONE )
        {
            // error! if this ever happens, debug it!
            fprintf( stderr,
                     "Btree::delete : can't steal or coalesce! debug me\n" );
            break;
        }

        if (whichsib_steal != SIB_NONE)
        {
            // steal! and disable coalescing.
            whichsib_coalesce = SIB_NONE;

            switch ( whichsib_steal )
            {
            case SIB_LEFT:
                // since we're going to the left, 
                // we have to adjust parentindex so that it
                // points to the data item which pivots l from r
                parentidx--;

                // first slide over items in curnod to make room for elt.
                for (i=ORDER_MO; i > 0; i--)
                {
                    if (i != ORDER_MO)
                    {
                        curn->keys[i] = curn->keys[i-1];
                        curn->datas[i] = curn->datas[i-1];
                    }
                    curn->ptrs[i] = curn->ptrs[i-1];
                }

                // pull down pivot from parent

                curn->keys[0] = parent->keys[parentidx];
                curn->datas[0] = parent->datas[parentidx];
                curn->numitems ++;

                // steal item from left sib and put in parent node.

                i = lsib->numitems;
                parent->keys[parentidx] = lsib->keys[i-1];
                parent->datas[parentidx] = lsib->datas[i-1];

                // and steal pointer too

                curn->ptrs[0] = lsib->ptrs[i];
                lsib->numitems--;

                // clear out now-unused slots in sibling

                lsib->keys[i-1] = NULL;
                lsib->datas[i-1] = 0;
                lsib->ptrs[i] = 0;

                lsib->mark_dirty();
                curn->mark_dirty();
                parent->mark_dirty();

                // done!
                break;

            case SIB_RIGHT:
                // pull down pivot item from parent down into curnod

                i = curn->numitems;
                curn->keys[i] = parent->keys[parentidx];
                curn->datas[i] = parent->datas[parentidx];

                // pull right sib's smallest item up into parent

                parent->keys[parentidx] = rsib->keys[0];
                parent->datas[parentidx] = rsib->datas[0];

                // grab lowest pointer in right sib for curnod

                curn->ptrs[i+1] = rsib->ptrs[0];
                curn->numitems ++;

                // slide down items in right sib

                for ( i = 0; i < ORDER_MO; i++ )
                {
                    if ( i != (ORDER_MO-1))
                    {
                        rsib->keys[i] = rsib->keys[i+1];
                        rsib->datas[i] = rsib->datas[i+1];
                    }
                    rsib->ptrs[i] = rsib->ptrs[i+1];
                }

                rsib->numitems --;

                // clear out unused slot in sib

                i = rsib->numitems;
                rsib->keys[i] = NULL;
                rsib->datas[i] = 0;
                rsib->ptrs[i+1] = 0;

                rsib->mark_dirty();
                curn->mark_dirty();
                parent->mark_dirty();

                // done!
                break;

            case SIB_NONE:
                // nothing; only here to satisfy compiler.
                break;
            }
        }

        if (whichsib_coalesce != SIB_NONE)
        {
            BTNode * l = NULL, * r = NULL;

            switch ( whichsib_coalesce )
            {
            case SIB_LEFT:
                l = lsib;
                r = curn;
                // since we're going to the left, 
                // we have to adjust parentindex so that it
                // points to the data item which pivots l from r
                parentidx--;
                break;

            case SIB_RIGHT:
                l = curn;
                r = rsib;
                // in this case parentindex already points to
                // the item which pivots l from r
                break;

            case SIB_NONE:
                // nothing; only here to satisfy compiler.
                break;
            }

            // slide items in r all the way to the right.

            int nr = r->numitems;
            int nl = l->numitems;

            int s = ORDER_MO - nr;
            for (i=nr; i >= 0; i--)
            {
                if (i != nr)
                {
                    r->keys[s+i] = r->keys[i];
                    r->datas[s+i] = r->datas[i];
                }
                r->ptrs[s+i] = r->ptrs[i];
            }

            // suck all items of l into left half
            // of r. adjust ptrs in parent.

            for ( i = 0; i < nl; i++ )
            {
                r->keys[i] = l->keys[i];
                r->datas[i] = l->datas[i];
                r->ptrs[i] = l->ptrs[i];
            }

            // there's one more pointer to get.
            // also get the pivot item from the parent.

            r->ptrs[i] = l->ptrs[i];
            r->keys[i] = parent->keys[parentidx];
            r->datas[i] = parent->datas[parentidx];

            // adjust item count.  this node is full!
            // (a half-full node plus a node which is 1 less than
            // half full, plus one item from parent node, makes a
            // full node.)

            r->numitems = ORDER_MO;

            // slide parent items around.

            for ( i = parentidx; i < (ORDER_MO-1); i++ )
            {
                parent->keys[i] = parent->keys[i+1];
                parent->datas[i] = parent->datas[i+1];
                parent->ptrs[i] = parent->ptrs[i+1];
            }

            // and one more ptr
            parent->ptrs[i] = parent->ptrs[i+1];

            // clear out unused positions

            parent->keys[i] = NULL;
            parent->datas[i] = 0;
            parent->ptrs[i+1] = 0;

            // if parent is root and size hits zero,
            // change root pointer in bti, and decrease depth

            parent->numitems--;
            if (parent->numitems == 0)
            {
                if (parent->root == false)
                {
                    // should not happen!
                    fprintf( stderr,
                             "Btree::delete : error! nonroot node shrunk!\n" );
                }
                oldrootfbn = nw->fbn;
                info.d->root_fbn.set( curfbn );
                info.d->depth.set( info.d->depth.get() - 1 );
                info.d->numnodes.set( info.d->numnodes.get() - 1 );
                r->root = true;
                oldrootfree = true;
            }

            parent->mark_dirty();
            curn->mark_dirty();

            // delete sib

            switch ( whichsib_coalesce )
            {
            case SIB_LEFT:
                node_cache->delete_node( l );
                lsib = NULL;
                break;

            case SIB_RIGHT:
                // can't delete l, because l is curnod.
                // instead, copy contents over and rename.
                for (i=0; i < r->numitems; i++)
                {
                    curn->keys[i] = r->keys[i];
                    curn->datas[i] = r->datas[i];
                    curn->ptrs[i] = r->ptrs[i];
                }
                // and one more ptr.
                curn->ptrs[i] = r->ptrs[i];
                curn->numitems = r->numitems;
                curn->leaf = r->leaf;
                curn->root = r->root;
                node_cache->delete_node( r );
                rsib = NULL;
                parent->ptrs[parentidx] = curn->get_fbn();
                break;

            case SIB_NONE:
                // satisfy bitchy compiler.
                break;
            }

            info.d->numnodes.set( info.d->numnodes.get() - 1 );
        }

        if (lsib)
            node_cache->release(lsib);
        if (rsib)
            node_cache->release(rsib);
    }

    while (nodes)
    {
        nw = nodes;
        nodes = nw->next;
        node_cache->release(nw->node);
        delete nw;
    }

    if (oldrootfree)
    {
        curn = node_cache->get(oldrootfbn);
        node_cache->delete_node(curn);
    }

    info.d->numrecords.set( info.d->numrecords.get() - 1 );
    info.mark_dirty();

    return ret;
}

bool
BtreeInternal :: iterate_node( BtreeIterator * bti, FB_AUID_T node_fbn )
{
    bool ret = true;
    BTNode * n;
    n = node_cache->get(node_fbn);

    bti->print( "node ID %08x:  %d items %s %s\n",
                node_fbn, n->numitems,
                n->root ? ", root" : "",
                n->leaf ? ", leaf" : "");
    int i;
    for (i=0; i < n->numitems; i++)
    {
        if (!n->leaf)
        {
            printf("node %08x recurse ptr %d -> ", node_fbn, i);
            if (iterate_node(bti, n->ptrs[i]) == false)
            {
                ret = false;
                break;
            }
        }
        printf("node %08x item %d: ", node_fbn, i);
        if (bti->handle_item( n->keys[i]->data,
                              n->keys[i]->keylen,
                              n->datas[i] ) == false)
        {
            ret = false;
            break;
        }
    }
    if (n->numitems > 0 && !n->leaf)
    {
        printf("node %08x recurse ptr %d -> ", node_fbn, i);
        if (iterate_node(bti, n->ptrs[i]) == false)
            ret = false;
    }
    node_cache->release(n);
    return true;
}

// virtual
bool
BtreeInternal :: iterate( BtreeIterator * bti )
{
    FB_AUID_T node_fbn;
    bool ret = true;
    iterate_inprogress = true;

    node_fbn = info.d->root_fbn.get();

    bti->print( "bti fbn : %08x\n"
                "root fbn : %08x\n"
                "numnodes : %d\n"
                "numrecords : %d\n"
                "depth : %d\n"
                "order : %d\n\n",
                info.d->bti_fbn.get(),
                node_fbn,
                info.d->numnodes.get(),
                info.d->numrecords.get(),
                info.d->depth.get(),
                info.d->order.get());

printf("root node: ");
    ret = iterate_node(bti, node_fbn);
    iterate_inprogress = false;

    return ret;
}
