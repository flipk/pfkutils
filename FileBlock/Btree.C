
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

int
BtreeInternal :: splitnode( BTNode * n, BTKey * key, UINT32 data_fbn,
                            UINT32 rightnode, int index )
{
    /** \todo implement */
    return 0;
}

//virtual
bool
BtreeInternal :: get( _BTDatum * key, _BTDatum * data )
{
    /** \todo implement */
    return false;
}

//virtual
bool
BtreeInternal :: put( _BTDatum * key, _BTDatum * data )
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
            if (iterate_node(bti, n->ptrs[i]) == false)
            {
                ret = false;
                break;
            }
        if (bti->handle_item( n->keys[i]->data,
                              n->keys[i]->keylen,
                              n->datas[i] ) == false)
        {
            ret = false;
            break;
        }
    }
    if (n->numitems > 0 && !n->leaf)
        if (iterate_node(bti, n->ptrs[i]) == false)
            ret = false;
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
