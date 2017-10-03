
#include "Btree.H"
#include "Btree_internal.H"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    BTNode  rootnode(_fbi);

    if (order < 0 || order > BtreeInternal::MAX_ORDER || 
        (order & 1) == 0)
        return false;

    root_fbn = _fbi->alloc( rootnode.d->node_size(order) );
    rootnode.get(root_fbn,true);
    rootnode.d->magic.set( _BTNode::MAGIC );
    rootnode.d->set_numitems( 0 );
    rootnode.d->set_root(true);
    rootnode.d->set_leaf(true);

    UINT32  info_fbn;
    BTInfo  info(_fbi);

    info_fbn = _fbi->alloc( sizeof(info) );
    info.get(info_fbn, true);
    info.d->magic.set ( _BTInfo::MAGIC );
    info.d->bti_fbn.set( info_fbn );
    info.d->root_fbn.set( root_fbn );
    info.d->numnodes.set( 1 );
    info.d->numrecords.set( 1 );
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
    BTNode node(fbi);
    node_size = node.d->node_size(BTREE_ORDER);
}

//virtual
BtreeInternal :: ~BtreeInternal(void)
{
    //xxx
}

//static
int
BtreeInternal :: compare_items( _BTDatum * one, _BTDatum * two )
{
    int onesize = one->get_size();
    int twosize = two->get_size();
    int comparelen = (onesize > twosize) ? twosize : onesize;
    int result;

    result = memcmp( one->get_ptr(), two->get_ptr(), comparelen );
    if (result != 0)
        return (result > 0) ? 1 : -1;
    if (onesize > twosize)
        return 1;
    else if (onesize < twosize)
        return -1;
    return 0;
}

//virtual
bool
BtreeInternal :: get( _BTDatum * key, _BTDatum * data )
{
    return false;
}

//virtual
bool
BtreeInternal :: put( _BTDatum * key, _BTDatum * data )
{
    return false;
}

//virtual
bool
BtreeInternal :: del( _BTDatum * key  )
{
    return false;
}

bool
BtreeInternal :: printnode( BtreePrintinfo * pi, UINT32 node_fbn )
{
    BTNode node(fbi);
    node.get(node_fbn);

    int numitems = node.d->get_numitems();
    pi->print( "node at %#x:  %d items\n",
               node_fbn, numitems);
    int i;
    for (i=0; i < numitems; i++)
    {
        if (printnode(pi, node.d->items[i].ptr.get()) == false)
            return false;
        if (pi->print_item(node.d->items[i].key.get(),
                           node.d->items[i].data.get()) == false)
            return false;
    }
    if (numitems > 0)
        printnode(pi, node.d->items[i].ptr.get());
    return true;
}

bool
BtreeInternal :: printinfo( BtreePrintinfo * pi )
{
    UINT32 node_fbn;

    node_fbn = info.d->root_fbn.get();

    pi->print( "bti fbn : 0x%x\n"
               "root fbn : 0x%x\n"
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

    return printnode(pi, node_fbn);
}
