
/** \file Btree.C
 * \brief implementation of Btree and BtreeInternal objects
 * \author Phillip F Knaack
 */

#include "Btree.H"
#include "Btree_internal.H"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** the FileBlockInterface::get_data_info_block string name
 * for the info field which describes the Btree. */
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

/** \see BtreeInternal::valid_file */
//static
bool
Btree :: valid_file( FileBlockInterface * fbi )
{
    return BtreeInternal::valid_file(fbi);
}

/** check if a file is a valid btree file.
 * this function checks a number of things: first that it can locate the
 * BTInfo block, then that the _BTInfo::MAGIC matches, then that the order
 * is sensible, then that the bti_fbn number matches.  If all these tests
 * pass, then the likelihood that this is a valid Btree file is pretty
 * high, so it returns true.
 * \param _fbi the FileBlockInterface to access the file.
 * \return true if the file appears to be a valid btree file,
 *              false if not. */
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

/** constructor of BtreeInternal class which takes the FileBlockInterface
 * as an argument.  This function attempts to locate and read the BTInfo
 * structure from the file and validate it.  It also learns the Btree order
 * and precalculates some constants.
 * \param _fbi  the interface to the file containing the btree
 */
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

/** compare two btree keys, and return -1, 0, or 1 depending on
 * what order they should be placed in.
 * \param one the first key to compare
 * \param two the second key to compare
 * \return -1 if one is "less than" two, 0 if one is binary equivalent
 *         to two, 1 if one is "greator than" two.
 */
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

/** compare key to each key in node.
 * if exact match is found, *exact is set to true.
 * \param n the node to walk through
 * \param key the key to compare to each key in the node
 * \param exact a pointer to a bool, which this function will set to
 *              true or false prior to returning to indicate if the key
 *              matches a key in the node exactly or not.
 * \return  index into node's items array; <ul> <li> if 'key' were
 *  inserted into node, this would be the index where it should go.
 *  <li> if this is a non-leaf, the pointer at that index should be
 * followed to get closer to the desired item. </ul>
 */
int
BtreeInternal :: walknode( _BTNode * n, _BTDatum * key, bool *exact )
{
    //xxx
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

/** recursive function which prints the contents of a node, and also
 * calls itself to follow node pointers and print children nodes.
 * also calls BtreePrintinfo::print_item for each data item as it goes.
 * as a consequence of this structure, print_item will receive all items
 * in the file exactly in key-order.  BtreePrintinfo::print_item should
 * return false, if it wants this algorithm to terminate before reaching
 * the end of the btree.
 * \param pi  a pointer to the user's BtreePrintinfo object
 * \param node_fbn  the FileBlock ID of the node to walk
 * \return true if the entire node was printed uninterrupted, or false 
 *         if BtreePrintinfo::print_item returned false, indicating that
 *         the entire tree-walk should be terminated. */
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
        if (!node.d->is_leaf())
            if (printnode(pi, node.d->items[i].ptr.get()) == false)
                return false;
        if (pi->print_item(node.d->items[i].key.get(),
                           node.d->items[i].data.get()) == false)
            return false;
    }
    if (numitems > 0 && !node.d->is_leaf())
        return printnode(pi, node.d->items[i].ptr.get());
    // else
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
