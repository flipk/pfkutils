
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
        res = compare_keys( n->keys[i], key );
        if (res <= 0)
            break;
    }

    *exact = (res == 0);

    return i;
}

UINT32
BtreeInternal :: splitnode( BTNode * n, BTKey ** key, UINT32 * data_fbn,
                            UINT32 rightptr, int index )
{
    int i;
    enum { R_PIVOT, R_LEFT, R_RIGHT } where = R_PIVOT;

    if (index < HALF_ORDER)
        // the key belongs in the left node; the pivot record for the two
        // nodes (to be promoted to the parent node) will be in the middle
        // of the old node.
        where = R_LEFT;
    else if (index == HALF_ORDER)
        // the provided key IS the pivot record, and will itself be promoted
        // to the parent node.
        where = R_PIVOT;
    else
        // the provided key belongs in the right node; the pivot record
        // to be promoted exists in the middle of the old node.
        where = R_RIGHT;

    // nr short for 'new right'
    BTNode * nr = node_cache->new_node();

    nr->leaf = n->leaf;
    nr->root = false;
    n->numitems = HALF_ORDER;
    nr->numitems = HALF_ORDER;

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
            n->keys[i + HALF_ORDER] = NULL;
            nr->datas[i] = n->datas[i + HALF_ORDER];
            n->datas[i + HALF_ORDER] = 0;
            nr->ptrs[i + 1] = n->ptrs[i + HALF_ORDER + 1];
            n->ptrs[i + HALF_ORDER + 1] = 0;
        }
        break;

    case R_LEFT:
        // copy right half of left node into right node

        for (i=0; i < HALF_ORDER; i++)
        {
            nr->keys[i] = n->keys[i + HALF_ORDER];
            n->keys[i + HALF_ORDER] = NULL;
            nr->datas[i] = n->datas[i + HALF_ORDER];
            n->datas[i + HALF_ORDER] = 0;
            nr->ptrs[i] = n->ptrs[i + HALF_ORDER];
            n->ptrs[i + HALF_ORDER] = 0;
        }
        // pick up one trailing ptr
        nr->ptrs[i] = n->ptrs[i + HALF_ORDER];

        // now slide over remaining components of left
        // node that must move to accomodate new item.

        for (i = HALF_ORDER-1; i >= index; i--)
        {
            n->keys[i+1] = n->keys[i];
            n->datas[i+1] = n->datas[i];
            n->ptrs[i+2] = n->ptrs[i+1];
        }

        // insert new item.

        n->keys[index] = *key;
        n->datas[index] = *data_fbn;
        n->ptrs[index+1] = rightptr;

        // pivot record to promote is left in leftnode.
        break;

    case R_RIGHT:
        // start copying things over to the right;
        // when we come to the place where the pivot is
        // supposed to go, insert it.

        index -= HALF_ORDER+1;
        int j = HALF_ORDER+1;

        nr->ptrs[0] = n->ptrs[j];
        n->ptrs[j] = 0;

        for (i=0; i < HALF_ORDER; i++)
        {
            if (i == index)
            {
                // this is where pivot rec goes.
                nr->keys[i] = *key;
                nr->datas[i] = *data_fbn;
                nr->ptrs[i+1] = rightptr;
                continue;
            }
            // else
            nr->keys[i] = n->keys[j];
            n->keys[j] = NULL;
            nr->datas[i] = n->datas[j];
            n->datas[j] = 0;
            nr->ptrs[i+1] = n->ptrs[j+1];
            n->ptrs[j+1] = 0;
            j++;
        }

        // pivot record to promote is left in the leftnode.
        break;
    }

    if (where != R_PIVOT)
    {
        // highest remaining element of leftnode becomes
        // the new promoted pivot record.
        *key = n->keys[HALF_ORDER];
        n->keys[HALF_ORDER] = NULL;
        *data_fbn = n->datas[HALF_ORDER];
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

    

    /** \todo implement */
    delete key;
    return ret;
}

//virtual
bool
BtreeInternal :: put( _BTDatum * _key, _BTDatum * data )
{
    if (iterate_inprogress)
    {
        fprintf(stderr, "ERROR: cannot modify database while iterate "
                "is in progress!\n");
        exit(1);
    }

    int keysz = _key->get_size();
    BTKey * key = new(keysz) BTKey(keysz);
    bool ret = false;

    

    /** \todo implement */
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

    bti->print( "node ID %08x:  %d items\n",
               node_fbn, n->numitems);
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

    ret = iterate_node(bti, node_fbn);
    iterate_inprogress = false;

    return ret;
}
