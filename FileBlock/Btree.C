
/** \file Btree.C
 * \brief implementation of Btree and BtreeInternal objects
 * \author Phillip F Knaack
 */

#include "Btree.H"
#include "Btree_internal.H"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>

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
    UINT32  root_fbn;

    if (order < 0 || order > BtreeInternal::MAX_ORDER || 
        (order & 1) == 0)
        return false;

    BTNode n(_fbi, order, 0);
    root_fbn = n.get_fbn();
    n.store();

    UINT32  info_fbn;
    BTInfo  info(_fbi);

    info_fbn = _fbi->alloc( sizeof(info) );
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
    UINT32 info_fbn;
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
}

//static
int
BtreeInternal :: compare_keys( BTKey * one, BTKey * two )
{
    int onesize = one->keylen;
    int twosize = two->keylen;
    int comparelen = (onesize > twosize) ? twosize : onesize;
    int result;

    result = memcmp( one->data, two->data, comparelen );
    if (result != 0)
        return (result > 0) ? 1 : -1;
    if (onesize > twosize)
        return 1;
    else if (onesize < twosize)
        return -1;
    return 0;
}

int
BtreeInternal :: walknode( BTNode * n, BTKey * key, bool *exact )
{
    if (n->numitems == 0)
    {
        *exact = false;
        return 0;
    }

    int i,res = 0;
    for (i=0; i < n->numitems; i++)
    {
        res = compare_keys( key, n->keys[i] );
        if (res <= 0)
            break;
    }

    *exact = (res == 0);

    return i;
}

UINT32
BtreeInternal :: splitnode( BTNode * n, BTKey ** key, UINT32 * data_fbn,
                            UINT32 rightptr, int idx )
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

    UINT32 nr_fbn = nr->get_fbn();
    node_cache->release(nr);
    return nr_fbn;
}

//virtual
bool
BtreeInternal :: get( _BTDatum * _key, _BTDatum * data )
{
    int keysz = _key->get_size();
    BTKey * key = new(keysz) BTKey(keysz);
    bool ret = false;
    bool exact;
    int idx;
    UINT32 curfbn;
    BTNode * curn;
    UINT32 data_fbn = 0;

    memcpy(key->data, _key->get_ptr(), keysz);

    curfbn = info.d->root_fbn.get();

    while (data_fbn == 0 && curfbn != 0)
    {
        curn = node_cache->get(curfbn);
        idx = walknode( curn, key, &exact );
        if (exact)
            data_fbn = curn->datas[idx];
        if (curn->leaf)
            curfbn = 0;
        else
            curfbn = curn->ptrs[idx];
        node_cache->release(curn);
    }

    if (data_fbn != 0)
    {
        FileBlock * fb = fbi->get(data_fbn);
        data->setfb(fb);
        ret = true;
    }

    delete key;
    return ret;
}

struct nodewalker {
    struct nodewalker * next;
    UINT32 fbn;
    BTNode * node;
    int idx;
    //
    nodewalker(struct nodewalker * _nxt, UINT32 _fbn,
               BTNode * _n, int _idx) {
        next = _nxt; fbn = _fbn; node = _n; idx = _idx;
    }
};

//virtual
bool
BtreeInternal :: put( _BTDatum * _key, _BTDatum * data, bool replace )
{
    if (iterate_inprogress)
    {
        fprintf(stderr, "ERROR: cannot modify database while iterate "
                "is in progress!\n");
        exit(1);
    }

    int i, keysz = _key->get_size();
    BTKey * key = new(keysz) BTKey(keysz);
    bool ret = false;
    nodewalker * nodes = NULL, * nw = NULL;
    UINT32 curfbn, right_fbn = 0, data_fbn;
    BTNode * curn;
    int curidx;
    FileBlock * fb = NULL;

    memcpy(key->data, _key->get_ptr(), keysz);

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
        curidx = walknode( curn, key, &exact );
        nodes = new nodewalker(nodes, curfbn, curn, curidx);

        if (exact)
        {
            // found an exact match in the tree.
            // realloc the data for this item and copy in
            // our new data but only if replace is set.

            if (replace == false)
                goto out;

            fbi->realloc(curn->datas[curidx], data->get_size());
            fb = fbi->get(curn->datas[curidx], /*for_write*/ true);
            memcpy(fb->get_ptr(), data->get_ptr(), data->get_size());
            fbi->release(fb);
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
    // if we overflow a node here, split the node and start
    // promoting up the tree. keep splitting and promoting up
    // until we reach a node where we're not overflowing a node.
    // if we reach the root and we overflow the root, split the
    // root and create a new root with the promoted item.

    data_fbn = fbi->alloc(data->get_size());
    fb = fbi->get(data_fbn, true);
    memcpy(fb->get_ptr(), data->get_ptr(), data->get_size());
    fbi->release(fb);
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

            curn->keys[curidx] = key;
            curn->datas[curidx] = data_fbn;
            curn->ptrs[curidx+1] = right_fbn;
            curn->mark_dirty();

            node_cache->release(curn);

            break;
        }

        // this node is full, so split it and prepare to 
        // promote a pivot record. split_node locates the pivot
        // and updates 'newr' to point to the pivot.

        right_fbn = splitnode( curn, &key, &data_fbn, right_fbn, curidx );

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
            newroot->keys[0] = key;
            newroot->datas[0] = data_fbn;
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

    /** \todo implement */

    if (ret == false)
        delete key;
    return ret;
}

//virtual
bool
BtreeInternal :: del( _BTDatum * key  )
{
    if (iterate_inprogress)
    {
        fprintf(stderr, "ERROR: cannot modify database while iterate "
                "is in progress!\n");
        exit(1);
    }

    /** \todo implement */

    return false;
}

bool
BtreeInternal :: iterate_node( BtreeIterator * bti, UINT32 node_fbn )
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
    UINT32 node_fbn;
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
