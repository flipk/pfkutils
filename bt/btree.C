
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "btree.H"

#if DLL2_INCLUDE_LOGNEW
#include "lognew.H"
#else
#define LOGNEW new
#endif

#undef  DEBUG

OBtree :: OBtree( FileBlockNumber * _nodes,
                FileBlockNumber * _keys,
                FileBlockNumber * _data )
    throw ( constructor_failed )
{
    fbn_nodes = _nodes;
    fbn_keys = _keys;
    fbn_data = _data;
    Btree_common();
}

OBtree :: OBtree( FileBlockNumber * _fbn )
    throw ( constructor_failed )
{
    fbn_nodes = _fbn;
    fbn_keys = _fbn;
    fbn_data = _fbn;
    Btree_common();
}

void
OBtree :: Btree_common( void )
{
    UINT32 bn;
    int len;

    // find btreeinfo node somewhere at the beginning
    // of the file.

    for ( bn = fbn_nodes->min_block_no(); bn < MAX_BTI_HUNT; bn++ )
    {
        bti = (btreeinfo*) fbn_nodes->get_block( bn, &len, &bti_magic );

        if ( bti == NULL )
            continue;

        if ( bti->magic == BTI_MAGIC )
            break;

        fbn_nodes->unlock_block( bti_magic, false );
    }

    constructor_failed::failure_reason res =
        constructor_failed :: NO_ERROR;

    if ( bn == MAX_BTI_HUNT )
        res = constructor_failed :: NO_NODES_FOUND;

    if ( res != constructor_failed :: NO_ERROR )
        throw constructor_failed( res );

    BTREE_ORDER = bti->order;
    HALF_ORDER = BTREE_ORDER / 2;
    ORDER_MO = BTREE_ORDER - 1;
}

OBtree :: ~OBtree( void )
{
    // the btreeinfo record should be the only
    // record still locked. if not, the fileblocknumber
    // will complain and make it obvious that something is wrong.

    fbn_nodes->unlock_block( bti_magic, true );
    delete fbn_nodes;
    if ( fbn_keys != fbn_nodes )
        delete fbn_keys;
    if ( fbn_data != fbn_keys && 
         fbn_data != fbn_nodes )
        delete fbn_data;
}

void
OBtree :: add_nodstor( node * _n, UINT32 _noderecno, int _index )
{
    nodstor * x = LOGNEW nodstor;
    x->n = _n;
    x->index = _index;
    x->noderecno = _noderecno;
    x->next = nods;
    nods = x;
}

OBtree :: node *
OBtree :: get_nodstor( UINT32 &_noderecno, int &_index )
{
    node * ret = NULL;
    nodstor * x = nods;
    if ( x != NULL )
    {
        ret    = x->n;
        _index = x->index;
        _noderecno = x->noderecno;
        nods   = x->next;
        delete x;
    }
    return ret;
}

// read in the file blocks holding a node.
// if this node is brand-new, instead of verifying 
// that the correct magic# exists, fill one in and 
// initialize the contents of the node.

OBtree :: node *
OBtree :: fetch_node( UINT32 blockno, bool newnode )
{
    int dummylen;
    node * r = LOGNEW node;
    r->dirty = false;
    r->nd = (_node*) fbn_nodes->get_block( blockno, &dummylen, &r->magic );
    if ( r->nd == NULL )
    {
        delete r;
        return NULL;
    }
    if ( newnode == true )
    {
        memset( r->nd, 0, node_size( bti->order ));
        r->nd->magic = NODE_MAGIC;
        r->dirty = true;
    }
    if (( r->nd->magic != NODE_MAGIC ) ||
        ( dummylen != node_size( bti->order )))
    {
        fbn_nodes->unlock_block( r->magic, false );
        delete r;
        return NULL;
    }
    return r;
}

void
OBtree :: unlock_node( node * n )
{
    fbn_nodes->unlock_block( n->magic, n->dirty );
    delete n;
}

// retrieve a record from the file.

OBtree :: rec *
OBtree :: fetch_rec( UINT32 keyblockno, UINT32 datablockno )
{
    rec * ret = LOGNEW rec;

    ret->key.dirty = false;
    ret->data.dirty = false;

    ret->key.recno = keyblockno;
    ret->data.recno = datablockno;

    if ( ret->key.recno != INVALID_BLK )
    {
        ret->key.ptr = fbn_keys->get_block( ret->key.recno,
                                       &ret->key.len,
                                       &ret->key.magic );
        if ( ret->key.ptr == NULL )
        {
            delete ret;
            return NULL;
        }
    }
    else
        ret->key.ptr = NULL;

    if ( ret->data.recno != INVALID_BLK )
    {
        ret->data.ptr = fbn_data->get_block( ret->data.recno,
                                        &ret->data.len,
                                        &ret->data.magic );
        if ( ret->data.ptr == NULL )
        {
            fbn_keys->unlock_block( ret->key.magic, false );
            delete ret;
            return NULL;
        }
    }
    else
        ret->data.ptr = NULL;

    return ret;
}

// static
void
OBtree :: new_file( FileBlockNumber * fbn, int order )
{
    btreeinfo * bti;
    UINT32 btibn, rbn;
    int sz;
    ULONG btimagic, rootmagic;
    _node * nd;

    if (( order & 1 ) == 0 )
    {
        printf( "OBtree::new_file: error! order supplied "
                "must be odd! (not %d)\n", order );
        return;
    }

    // allocate and populate a btreeinfo struct
    // and a new node for rootnode in the file.

    btibn = fbn->alloc( sizeof( btreeinfo ));
    rbn   = fbn->alloc( node_size( order ));

    if ( btibn > MAX_BTI_HUNT )
    {
        fbn->free( btibn );
        printf( "OBtree::new_file: error! first block number "
                "free is %d,\nwhich is too high for this "
                "API to find! (> %d)\n",
                btibn, MAX_BTI_HUNT );
        return;
    }

    bti = (btreeinfo*) fbn->get_block( btibn, &sz, &btimagic );
    nd = (_node*) fbn->get_block( rbn, &sz, &rootmagic );

    memset( nd, 0, node_size( order ));

    nd->magic = NODE_MAGIC;
    nd->set_leaf();
    nd->set_root();

    bti->magic = BTI_MAGIC;
    bti->bti_recno = btibn;
    bti->depth = 1;
    bti->order = order;
    bti->rootblockno = rbn;
    bti->numnodes = 1;
    bti->numrecords = 0;

    fbn->unlock_block( btimagic, true );
    fbn->unlock_block( rootmagic, true );

    fbn->flush();
}

void
OBtree :: dumptree( btree_printinfo * pi )
{
    if ( pi->options & btree_printinfo::BTREE_INFO )
    {
        pi->print( (char*)"bti: \n"
                   "  recno = %d\n"
                   "  rootblockno = %d\n"
                   "  numnodes = %d\n"
                   "  numrecords = %d\n"
                   "  depth = %d\n"
                   "  order = %d\n",
                   bti->bti_recno,  bti->rootblockno, bti->numnodes,
                   bti->numrecords, bti->depth,       bti->order     );
    }
    dumpnode( pi, bti->rootblockno );
}

bool
OBtree :: dumpnode( btree_printinfo * pi, int recno )
{
    int i;
    node *n = fetch_node( recno );
    if ( pi->options & btree_printinfo::NODE_INFO )
    {
        pi->print( (char*)"node at %d:\n"
                   "  numitems = %d (%s %s)\n"
                   "  data =    ",
                   recno, n->nd->get_numitems(),
                   n->nd->is_root() ? "root" : "-", 
                   n->nd->is_leaf() ? "leaf" : "-" );
        for ( i = 0; i < n->nd->get_numitems(); i++ )
            pi->print( (char*)"%03d/%03d   ", n->nd->d[i].key, n->nd->d[i].data );
        pi->print( (char*)"\n  ptrs = " );
        for ( i = 0; i <= n->nd->get_numitems(); i++ )
            pi->print( (char*)"%03d       ", n->nd->d[i].ptr );
        pi->print( (char*)"\n" );
    }
    for ( i = 0; i < n->nd->get_numitems(); i++ )
    {
        if ( !n->nd->is_leaf() )
            if ( dumpnode( pi, n->nd->d[i].ptr ) == false )
            {
                unlock_node( n );
                return false;
            }
        rec * r = fetch_rec( pi->options & btree_printinfo::KEY_REC_PTR ?
                             n->nd->d[i].key : INVALID_BLK,
                             pi->options & btree_printinfo::DATA_REC_PTR ?
                             n->nd->d[i].data : INVALID_BLK );
        char * s = pi->sprint_element(
            recno,
            n->nd->d[i].key, r->key.ptr, r->key.len,
            n->nd->d[i].data, r->data.ptr, r->data.len,
            &r->data.dirty );

        if ( s == NULL )
        {
            unlock_rec( r );
            unlock_node( n );
            return false;
        }
        pi->print( (char*)"%s", s );
        pi->sprint_element_free( s );
        unlock_rec( r );
    }
    if ( !n->nd->is_leaf() )
        if ( dumpnode( pi, n->nd->d[i].ptr ) == false )
        {
            unlock_node( n );
            return false;
        }
    unlock_node( n );
    return true;
}

// this returns a rec where ptrs are valid and fileblocks are locked.

OBtree :: rec *
OBtree :: alloc_rec( int keylen, int datalen )
{
    rec * ret = LOGNEW rec;

    ret->key.len = keylen;
    ret->key.recno = fbn_keys->alloc( keylen );
    ret->key.ptr = fbn_keys->get_block( ret->key.recno,
                                   &keylen, &ret->key.magic );
    ret->key.dirty = true;

    ret->data.len = datalen;
    ret->data.recno = fbn_data->alloc( datalen );
    ret->data.ptr = fbn_data->get_block( ret->data.recno,
                                    &datalen, &ret->data.magic );
    ret->data.dirty = true;

    return ret;
}

// this unlocks the file blocks and frees the rec.
// note that a key or data where either the recno is
// not present or the ptr is not populated is OK.
// this comes in useful in optimizing some algorithms
// below where only part of the information in the rec
// is actually needed.

void
OBtree :: unlock_rec( rec * r )
{
    if (( r->key.recno != INVALID_BLK ) && ( r->key.ptr != NULL ))
        fbn_keys->unlock_block( r->key.magic,  r->key.dirty  );

    if (( r->data.recno != INVALID_BLK ) && ( r->data.ptr != NULL ))
        fbn_data->unlock_block( r->data.magic, r->data.dirty );

    delete r;
}

OBtree :: rec *
OBtree :: get_rec( UCHAR *keyptr, int keylen )
{
    rec r;
    bool exact;
    UINT32 noderecno;
    UINT32 keyrecno  = INVALID_BLK;
    UINT32 datarecno = INVALID_BLK;
    int index;

    r.key.len = keylen;
    r.key.ptr = keyptr;

    // start at root node.
    // walk each node, if exact match found we're done.
    // if exact not found, follow ptr to next level,
    // until leaf level reached.

    noderecno = bti->rootblockno;
    while (( keyrecno == INVALID_BLK ) && ( noderecno != 0 ))
    {
        node * n = fetch_node( noderecno );
        index = walk_node( n, &r, exact );
        if ( exact )
        {
            keyrecno  = n->nd->d[index].key;
            datarecno = n->nd->d[index].data;
        }

        if ( n->nd->is_leaf() )
            noderecno = 0;
        else
            noderecno = n->nd->d[index].ptr;

        unlock_node( n );
    }

    rec * ret = NULL;

    if ( keyrecno != INVALID_BLK )
        ret = fetch_rec( keyrecno, datarecno );

    return ret;
}

// compare two records. buffers compared a byte at a time.
// if a short record matches the start of a longer record,
// preference goes to the longer. (only key portion of recs
// are compared.)

int
OBtree :: compare_recs( rec *a, rec *b )
{
    int comparelen = (a->key.len > b->key.len) ?
        b->key.len : a->key.len;

    int res = memcmp( a->key.ptr, b->key.ptr, comparelen );

    if ( res != 0 )
        return (res > 0) ? 1 : -1;

    if ( a->key.len > b->key.len )
        return 1;
    else if ( a->key.len < b->key.len )
        return -1;
    return 0;
}

// compare key of 'r' to each rec in node 'n'. 
// if exact match is found, 'exact' is set to true.
// return value is index into node's node_data array;
// if 'r' were inserted into 'n', it would be at that index.
// if this is a non-leaf, the pointer at that index should be
// followed to get closer to rec 'r'.

int
OBtree :: walk_node( node * n, rec * r, bool &exact )
{
    int i;
    int cmpres = 0;
    int max = n->nd->get_numitems();

    if ( max == 0 )
    {
        exact = false;
        return 0;
    }

    for ( i = 0; i < max; i++ )
    {
        rec tmp;
        UINT32 bn = n->nd->d[i].key;
        // compare_recs only needs the key portion
        // so optimize by not fetching the data portion
        tmp.key.recno = bn;
        tmp.key.ptr = fbn_keys->get_block( bn, &tmp.key.len,
                                      &tmp.key.magic );
        cmpres = compare_recs( r, &tmp );
        fbn_keys->unlock_block( tmp.key.magic, false );
        if ( cmpres <= 0 )
            break;
    }

    exact = (cmpres == 0) ? true : false;
    return i;
}

// take full node plus 1 record and split all the records
// into two nodes, with a record which sits in between
// the two nodes.
// actually the old node becomes the new left node,
// and we create a new node for the right. replace
// the information inside the recptr with the rec
// which is to be promoted.
// in the process, don't forget about the new record's rightnode pointer.

int
OBtree :: split_node( node *n, rec *r, UINT32 rightnode, int index )
{
    node *right;
    UINT32 newrightnode;
    int i;

    bti->numnodes++;
    newrightnode = fbn_nodes->alloc( node_size( bti->order ));
    right = fetch_node( newrightnode, true );

    right->dirty = true;
    n->dirty = true;

    right->nd->set_numitems( HALF_ORDER );
    n->nd->set_numitems( HALF_ORDER );
    if ( n->nd->is_leaf())
        right->nd->set_leaf();
    right->nd->clear_root();

    // determine if our new rec is the pivot,
    // if it belongs in the left node, or in the right node.

    enum { R_PIVOT, R_LEFT, R_RIGHT } where;

    if ( index < HALF_ORDER )
        where = R_LEFT;
    else if ( index == HALF_ORDER )
        where = R_PIVOT;
    else
        where = R_RIGHT;

    // the way the recs are moved around
    // depends on where r goes. if r is not
    // the pivot, the pivot is intentionally
    // left as the rightmost rec of the leftnode,
    // so that it can be collected after this switch.

    switch ( where )
    {
    case R_PIVOT:
        // the new node is the pivot, so don't update 
        // r; just split leftnode into rightnode and 
        // we're done. right half of nodes in leftnode
        // plus their rightpointers are copied. the first left
        // ptr of the new rightnode points to the
        // passed-in rightnode.

        right->nd->d[0].ptr = rightnode;
        for ( i = 0; i < HALF_ORDER; i++ )
        {
            right->nd->d[i].key = n->nd->d[ i + HALF_ORDER ].key;
            n->nd->d[ i + HALF_ORDER ].key = 0;
            right->nd->d[i].data = n->nd->d[ i + HALF_ORDER ].data;
            n->nd->d[ i + HALF_ORDER ].data = 0;
            right->nd->d[i+1].ptr = n->nd->d[ i + HALF_ORDER + 1 ].ptr;
            n->nd->d[ i + HALF_ORDER + 1 ].ptr = 0;
        }
        break;

    case R_LEFT:
        // copy right half of left node into right node.

        for ( i = 0; i < HALF_ORDER; i++ )
        {
            right->nd->d[i].key = n->nd->d[ i + HALF_ORDER ].key;
            n->nd->d[ i + HALF_ORDER ].key = 0;
            right->nd->d[i].data = n->nd->d[ i + HALF_ORDER ].data;
            n->nd->d[ i + HALF_ORDER ].data = 0;
            right->nd->d[i].ptr = n->nd->d[ i + HALF_ORDER ].ptr;
            n->nd->d[ i + HALF_ORDER ].ptr = 0;
        }
        right->nd->d[i].ptr = n->nd->d[ i + HALF_ORDER ].ptr;

        // then slide over remaining components of left
        // node that must move to accommodate new item.

        for ( i = HALF_ORDER-1; i >= index; i-- )
        {
            n->nd->d[i+1].key  = n->nd->d[i].key;
            n->nd->d[i+1].data = n->nd->d[i].data;
            n->nd->d[i+2].ptr  = n->nd->d[i+1].ptr;
        }

        // then insert new item.

        n->nd->d[index].key  = r->key.recno;
        n->nd->d[index].data = r->data.recno;
        n->nd->d[index+1].ptr = rightnode;

        // pivot is left in leftnode.

        break;

    case R_RIGHT:
        // start copying things over to the right;
        // when we come to the place where newr is 
        // supposed to go, insert it.

        index -= HALF_ORDER+1;
        int j = HALF_ORDER+1;

        right->nd->d[0].ptr = n->nd->d[j].ptr;
        n->nd->d[j].ptr = 0;

        for ( i = 0; i < HALF_ORDER; i++ )
        {
            if ( i == index )
            {
                // this is where newrec goes.
                right->nd->d[i].key  = r->key.recno;
                right->nd->d[i].data = r->data.recno;
                right->nd->d[i+1].ptr = rightnode;
                continue;
            }
            // else
            right->nd->d[i].key   = n->nd->d[j].key;
            right->nd->d[i].data  = n->nd->d[j].data;
            right->nd->d[i+1].ptr = n->nd->d[j+1].ptr;
            n->nd->d[j].key   = 0;
            n->nd->d[j].data  = 0;
            n->nd->d[j+1].ptr = 0;
            j++;
        }

        // pivot rec is left in leftnode.

        break;
    }

    if ( where != R_PIVOT )
    {
        // highest remaining element from leftnode
        // becomes the promoted rec.

        // abbreviated versions of unlock_rec and fetch_rec here.

        if (( r->key.recno != INVALID_BLK ) && ( r->key.ptr != NULL ))
            fbn_keys->unlock_block( r->key.magic,  r->key.dirty  );

        if (( r->data.recno != INVALID_BLK ) && ( r->data.ptr != NULL ))
            fbn_data->unlock_block( r->data.magic, r->data.dirty );

        // the second half of the put() method and the 
        // split_node() algorithm do not require the pointer
        // or magic to be valid in the pivot rec; only the
        // recnos need to be present, because the walk_node()s
        // in the first half of put() already calculated the
        // indexes into the respective nodes.

        r->key.recno  = n->nd->d[HALF_ORDER].key;
        r->key.ptr    = NULL;
        r->key.magic  = 0;
        r->key.dirty  = false;

        r->data.recno = n->nd->d[HALF_ORDER].data;
        r->data.ptr   = NULL;
        r->data.magic = 0;
        r->data.dirty = false;

        n->nd->d[HALF_ORDER].key  = 0;
        n->nd->d[HALF_ORDER].data = 0;
    }

    unlock_node( right );
    return newrightnode;
}

OBtree :: put_retval
OBtree :: put_rec( rec * newr )
{
    UINT32 curnod_blockno = bti->rootblockno;
    node * curnod = fetch_node( curnod_blockno );
    int index, i;
    UINT32 rightnode = 0;
    put_retval ret = PUT_FAIL;

    nods = NULL;

    // start walking down from the rootnode a level at a time,
    // until we either find an exact match or we hit a leaf.
    // each time we go down a level, add an element to the nodstor
    // linked-list (head-first) so that we can trace our path
    // back up the tree if we need to split and promote.

    while ( 1 )
    {
        bool exact;
        index = walk_node( curnod, newr, exact );

        add_nodstor( curnod, curnod_blockno, index );

        if ( exact )
        {
            // found an exact match in the tree.
            // delete old data portion, splice in our
            // new data portion, and delete our new key
            // since its really a duplicate of the old key.

            fbn_data->free( curnod->nd->d[index].data );
            curnod->nd->d[index].data = newr->data.recno;
            fbn_keys->unlock_block( newr->key.magic, false );
            fbn_keys->free( newr->key.recno );
            newr->key.recno = INVALID_BLK;
            curnod->dirty = true;
            ret = PUT_OVERWRITE;
            goto out;
        }

        // break out if we're at the leaf node, or
        // proceed to the next level down the tree.

        if ( curnod->nd->is_leaf() )
            break;

        curnod_blockno = curnod->nd->d[index].ptr;
        curnod = fetch_node( curnod_blockno );
    }

    // if we hit this point that means we haven't seen an
    // exact match, so we should insert at the leaf level.
    // if we overflow a node here, split the node and start
    // promoting up the tree. keep splitting and promoting up
    // until we reach a node where we're not overflowing a node.
    // if we reach the root and we overflow the root, split the
    // root and create a new root with the promoted item.

    // note that beyond this point, the pointers and magics
    // in 'newr' are no longer needed, because the above code
    // has already calculated and stored the indexes into 
    // the nodes above. thus, split_node() doesn't populate
    // those members while promoting pivots.

    curnod = get_nodstor( curnod_blockno, index );
    bti->numrecords++;
    ret = PUT_NEW;

    while ( 1 )
    {
        int numi = curnod->nd->get_numitems();
        node * old_curnod;
        if ( numi < ORDER_MO )
        {
            // this node is not full, so we can insert
            // into this node at the appropriate index,
            // and be done. slide over recs in this node
            // to make room for the new rec.

            curnod->nd->set_numitems( numi + 1 );
            for ( i = ORDER_MO; i > index; i-- )
                curnod->nd->d[i].ptr = curnod->nd->d[i-1].ptr;

            for ( i = ORDER_MO-1; i >= index; i-- )
            {
                _node::node_data * d1 = &curnod->nd->d[i];
                _node::node_data * d2 = &curnod->nd->d[i-1];
                d1->key  = d2->key;
                d1->data = d2->data;
            }

            curnod->nd->d[index].key = newr->key.recno;
            curnod->nd->d[index].data = newr->data.recno;
            curnod->nd->d[index+1].ptr = rightnode;
            curnod->dirty = true;
            unlock_node( curnod );
            break;
        }

        // this node is full, so split it and prepare to 
        // promote a pivot record. split_node locates the pivot
        // and updates 'newr' to point to the pivot.

        rightnode = split_node( curnod, newr, rightnode, index );

        old_curnod = curnod;
        curnod = get_nodstor( curnod_blockno, index );

        if ( curnod == NULL )
        {
            // we're at the root node, so we must create
            // a new root node and insert the pivot record
            // by itself here.

            UINT32 newrootbn = fbn_nodes->alloc( node_size( bti->order ));
            node * newroot = fetch_node( newrootbn, true );
            newroot->dirty = true;
            newroot->nd->set_numitems( 1 );
            newroot->nd->d[0].ptr = bti->rootblockno;
            newroot->nd->d[1].ptr = rightnode;
            newroot->nd->d[0].key = newr->key.recno;
            newroot->nd->d[0].data = newr->data.recno;
            newroot->nd->set_root();
            old_curnod->nd->clear_root();
            unlock_node( old_curnod );
            bti->rootblockno = newrootbn;
            bti->numnodes++;
            bti->depth++;
            unlock_node( newroot );

            break;
        }

        unlock_node( old_curnod );
    }

 out:
    while ( 1 )
    {
        curnod = get_nodstor( curnod_blockno, index );
        if ( curnod == NULL )
            break;
        unlock_node( curnod );
    }

    newr->key.dirty = true;
    newr->data.dirty = true;
    unlock_rec( newr );

    return ret;
}

OBtree :: delete_retval
OBtree :: delete_rec( UCHAR * keyptr, int keylen )
{
    rec * r;

    r = LOGNEW rec;
    r->key.ptr = keyptr;
    r->key.len = keylen;
    r->key.magic = 0;
    r->key.recno = INVALID_BLK;

    r->data.ptr = NULL;
    r->data.len = 0;
    r->data.magic = 0;
    r->data.recno = INVALID_BLK;

    return delete_rec( r );
}

OBtree :: delete_retval
OBtree :: delete_rec( rec * del_rec )
{
    UINT32 curnod_blockno;
    node * curnod;
    int index, i;
    delete_retval ret;
    bool exact;
    node * foundn;
    int foundnindex;

    curnod_blockno = bti->rootblockno;
    curnod = fetch_node( curnod_blockno );
    ret = DELETE_FAIL;
    exact = false;
    nods = NULL;

    // begin walking down the tree looking for the
    // item. at each node, store a pointer and index
    // to the node in the nodstore.
    // if we find an exact match, bail out even if its
    // not the leaf.

    while ( 1 )
    {
        index = walk_node( curnod, del_rec, exact );

        add_nodstor( curnod, curnod_blockno, index );
#ifdef DEBUG
        printf( "1. add node %d index %d to nodstor\n",
                curnod_blockno, index );
#endif
        if ( exact || curnod->nd->is_leaf() )
            break;

        curnod_blockno = curnod->nd->d[index].ptr;
        curnod = fetch_node( curnod_blockno );
    }

    unlock_rec( del_rec );

    // if we didn't find an exact match, unlock all nodes
    // in the nodstore and return failure code.

    if ( !exact )
    {
        while ( 1 )
        {
            curnod = get_nodstor( curnod_blockno, index );
            if ( curnod == NULL )
                break;
            unlock_node( curnod );
        }

        return DELETE_KEY_NOT_FOUND;
    }

    // go to the right on this last node.
    nods->index++;

    // store the ref to the node which contains the deleted item.
    // also, free the space used by the item.

    foundn = curnod;
    foundnindex = index;

#ifdef DEBUG
    printf( "index=%d free key at %d and data at %d\n",
            index,
            curnod->nd->d[index].key, 
            curnod->nd->d[index].data );
#endif
    fbn_keys->free( curnod->nd->d[index].key  );
    fbn_data->free( curnod->nd->d[index].data );

    nodstor * ndstptr;

    // if we're not at the leaf, continue walking down to the 
    // leaf, and store the path we used to get there. the first
    // time, go to the right, from then on go to leftmost.
    // this way we can end up at the smallest node to the right
    // of the target (for pulling up).

    bool _first = true;
    while ( !curnod->nd->is_leaf() )
    {
        int fetchwhich = 0;
        if ( _first )
        {
            _first = false;
            fetchwhich = index + 1;
        }
        UINT32 curnod_blockno = curnod->nd->d[ fetchwhich ].ptr;
        curnod = fetch_node( curnod_blockno );
        add_nodstor( curnod, curnod_blockno, 0 );
#ifdef DEBUG
        printf( "2. add node %d index %d to nodstor\n",
                curnod_blockno, 0 );
#endif
        index = 0;
    }

    // if we've walked down to a leaf where the deleted item
    // was in a nonleaf, pull up the smallest leaf item into
    // the deleted item's slot.

    if ( foundn != curnod )
    {
        foundn->nd->d[ foundnindex ].key  = curnod->nd->d[ index ].key;
        foundn->nd->d[ foundnindex ].data = curnod->nd->d[ index ].data;
        foundn->dirty = true;
    }

    // now in the leaf, slide over the remaining items 
    // to take up the space of the item that was 
    // removed or deleted.
    // don't need to mess with ptrs because we know
    // that this is a leaf.

    for ( i = index; i < ORDER_MO; i++ )
    {
        curnod->nd->d[ i ].key =  curnod->nd->d[ i+1 ].key;
        curnod->nd->d[ i ].data = curnod->nd->d[ i+1 ].data;
    }

    // clear out the right-hand slot left behind by the slide.

    curnod->nd->d[ ORDER_MO-1 ].key = 0;
    curnod->nd->d[ ORDER_MO-1 ].data = 0;
    curnod->nd->decr_numitems();
    curnod->dirty = true;

    ret = DELETE_OK;

    bool oldrootfree = false;
    UINT32 oldrootblockno = 0;

    // start analyzing nodes for redistribution.
    // return back up the list of nodes until we 
    // no longer need to steal or coalesce.

    ndstptr = nods;
    while ( 1 )
    {
        node * parent;
        node * l_sibling = NULL;
        node * r_sibling = NULL;
        UINT32 l_sibblockno = 0;
        UINT32 r_sibblockno = 0;
        int parentindex;
        enum sibwho { SIB_NONE, SIB_LEFT, SIB_RIGHT };
        sibwho whichsib_steal = SIB_NONE;
        sibwho whichsib_coalesce = SIB_NONE;

        curnod = ndstptr->n;
        curnod_blockno = ndstptr->noderecno;

        if (( curnod->nd->get_numitems() >= HALF_ORDER ) ||
            ( curnod->nd->is_root() ))
        {
            break;
        }

        ndstptr = ndstptr->next;
        if ( ndstptr == NULL )
        {
            // actually this should never happen; the above case
            // should happen first.  but handle it anyway.
            printf( "OBtree::delete : case that should never happen!\n" );
            break;
        }

        parent = ndstptr->n;
        parentindex = ndstptr->index;

#ifdef DEBUG
        printf( "curnod = %d, parent = %d, parentindex = %d\n",
                curnod_blockno, ndstptr->noderecno, parentindex );
#endif

        // check left sib -- if more than half full we can use 
        // left sib's rightmost entry to steal; if its only
        // exactly half full, mark it as candidate for coalescing.

        // first check that we actually have a left sib.
        if ( parentindex > 0 )
        {
            l_sibblockno = parent->nd->d[ parentindex-1 ].ptr;
#ifdef DEBUG
            printf( "left sib %d\n", l_sibblockno );
#endif
            l_sibling = fetch_node( l_sibblockno );
            if ( l_sibling )
            {
                // actually, l_sibling==NULL is an error and
                // should never happen, but the if check is nice anyway.
                if ( l_sibling->nd->get_numitems() > HALF_ORDER )
                    whichsib_steal = SIB_LEFT;
                else
                    whichsib_coalesce = SIB_LEFT;
            }
        }

        // check right sib -- if more than half full we can use
        // right sib's leftmost entry to steal. if its only half
        // full, mark it as candidate for coalescing.

        // if we can steal from the left, don't bother
        // looking to the right. then check that we
        // actually have a right sib.
        if ( whichsib_steal == SIB_NONE &&
             parentindex < parent->nd->get_numitems() )
        {
            r_sibblockno = parent->nd->d[ parentindex+1 ].ptr;
#ifdef DEBUG
            printf( "right sib %d at index %d (ni was %d)\n",
                    r_sibblockno, parentindex+1,
                    parent->nd->get_numitems() );
#endif
            r_sibling = fetch_node( r_sibblockno );
            if ( r_sibling )
            {
                if ( r_sibling->nd->get_numitems() > HALF_ORDER )
                    whichsib_steal = SIB_RIGHT;
                else
                    whichsib_coalesce = SIB_RIGHT;
            }
        }

        if ( whichsib_steal == SIB_NONE && 
             whichsib_coalesce == SIB_NONE )
        {
            // error! if this ever happens, debug it!
            printf( "OBtree::delete : can't steal or coalesce! debug me\n" );
            ret = DELETE_FAIL;
            break;
        }

#ifdef DEBUG
        printf( "steal %s\n",
                (whichsib_steal == SIB_NONE) ? "NONE" :
                (whichsib_steal == SIB_LEFT) ? "LEFT" : "RIGHT" );
#endif

        if ( whichsib_steal != SIB_NONE )
        {
            // steal! and disable coalescing
            whichsib_coalesce = SIB_NONE;

            switch ( whichsib_steal )
            {
            case SIB_LEFT:
                // since we're going to the left, 
                // we have to adjust parentindex so that it
                // points to the data item which pivots l from r
                parentindex--;

                // first slide over items in curnod to make room for elt.
                for ( i = ORDER_MO; i > 0; i-- )
                {
                    if ( i != ORDER_MO )
                    {
                        curnod->nd->d[ i ].key  = curnod->nd->d[ i-1 ].key;
                        curnod->nd->d[ i ].data = curnod->nd->d[ i-1 ].data;
                    }
                    curnod->nd->d[ i ].ptr  = curnod->nd->d[ i-1 ].ptr;
                }

                // pull down pivot item from parent

                curnod->nd->d[ 0 ].key  = parent->nd->d[ parentindex ].key;
                curnod->nd->d[ 0 ].data = parent->nd->d[ parentindex ].data;
                curnod->nd->incr_numitems();

                // steal item from left sib and put in parent node.

                i = l_sibling->nd->get_numitems();
                parent->nd->d[ parentindex ].key  =
                    l_sibling->nd->d[ i-1 ].key;
                parent->nd->d[ parentindex ].data  =
                    l_sibling->nd->d[ i-1 ].data;

                // and steal pointer, too

                curnod->nd->d[ 0 ].ptr = 
                    l_sibling->nd->d[ i ].ptr;
                l_sibling->nd->decr_numitems();

                // clear out now-unused slots in sibling

                l_sibling->nd->d[ i-1 ].key = 0;
                l_sibling->nd->d[ i-1 ].data = 0;
                l_sibling->nd->d[ i ].ptr = 0;

                l_sibling->dirty = true;
                curnod->dirty = true;
                parent->dirty = true;

                // done!

                break;

            case SIB_RIGHT:
                // pull down pivot item from parent down into curnod

                i = curnod->nd->get_numitems();
                curnod->nd->d[ i ].key  = parent->nd->d[ parentindex ].key;
                curnod->nd->d[ i ].data = parent->nd->d[ parentindex ].data;

                // pull right sib's smallest item up into parent

                parent->nd->d[ parentindex ].key  = r_sibling->nd->d[ 0 ].key;
                parent->nd->d[ parentindex ].data = r_sibling->nd->d[ 0 ].data;

                // grab lowest pointer in right sib for curnod

                curnod->nd->d[ i+1 ].ptr = r_sibling->nd->d[ 0 ].ptr;

                curnod->nd->incr_numitems();

                // slide down items in right sib

                for ( i = 0; i < ORDER_MO; i++ )
                {
                    if ( i != (ORDER_MO-1))
                    {
                        r_sibling->nd->d[ i ].key  =
                            r_sibling->nd->d[ i+1 ].key;
                        r_sibling->nd->d[ i ].data =
                            r_sibling->nd->d[ i+1 ].data;
                    }
                    r_sibling->nd->d[ i ].ptr = r_sibling->nd->d[ i+1 ].ptr;
                }

                r_sibling->nd->decr_numitems();

                // clear out unused slot in sib

                i = r_sibling->nd->get_numitems();
                r_sibling->nd->d[ i ].key  = 0;
                r_sibling->nd->d[ i ].data = 0;
                r_sibling->nd->d[ i+1 ].ptr = 0;

                r_sibling->dirty = true;
                curnod->dirty = true;
                parent->dirty = true;

                // done!

                break;

            case SIB_NONE:
                // nothing
                break;
            }
        }

#ifdef DEBUG
        printf( "coalesce %s\n",
                (whichsib_coalesce == SIB_NONE) ? "NONE" :
                (whichsib_coalesce == SIB_LEFT) ? "LEFT" : "RIGHT" );
#endif

        if ( whichsib_coalesce != SIB_NONE )
        {
            node * l = NULL, * r = NULL;

            switch ( whichsib_coalesce )
            {
            case SIB_LEFT:
                l = l_sibling;
                r = curnod;
                // since we're going to the left, 
                // we have to adjust parentindex so that it
                // points to the data item which pivots l from r
                parentindex--;
                break;

            case SIB_RIGHT:
                l = curnod;
                r = r_sibling;
                // in this case parentindex already points to
                // the item which pivots l from r
                break;

            case SIB_NONE:
                // nothing
                break;
            }

            // slide items in r all the way to the right.

            int nr = r->nd->get_numitems();
            int nl = l->nd->get_numitems();

            int s = ORDER_MO - nr;
            for ( i = nr; i >= 0; i-- )
            {
                if ( i != nr )
                {
                    r->nd->d[ s+i ].key  = r->nd->d[ i ].key;
                    r->nd->d[ s+i ].data = r->nd->d[ i ].data;
                }
                r->nd->d[ s+i ].ptr  = r->nd->d[ i ].ptr;
            }


            // suck all items of l into left half
            // of r. adjust ptrs in parent.

            for ( i = 0; i < nl; i++ )
            {
                r->nd->d[ i ].key  = l->nd->d[ i ].key;
                r->nd->d[ i ].data = l->nd->d[ i ].data;
                r->nd->d[ i ].ptr  = l->nd->d[ i ].ptr;
            }

            // there's one more pointer to get.
            // also get the pivot item from the parent.

            r->nd->d[ i ].ptr  = l->nd->d[ i ].ptr;
            r->nd->d[ i ].key  = parent->nd->d[ parentindex ].key;
            r->nd->d[ i ].data = parent->nd->d[ parentindex ].data;

            // adjust item count.  this node is full!
            // (a half-full node plus a node which is 1 less than
            // half full, plus one item from parent node, makes a
            // full node.)

            r->nd->set_numitems( ORDER_MO );

            // slide parent items around.

            for ( i = parentindex; i < (ORDER_MO-1); i++ )
            {
                parent->nd->d[ i ].key  = parent->nd->d[ i+1 ].key;
                parent->nd->d[ i ].data = parent->nd->d[ i+1 ].data;
                parent->nd->d[ i ].ptr  = parent->nd->d[ i+1 ].ptr;
            }

            // and one more ptr.
            parent->nd->d[ i ].ptr  = parent->nd->d[ i+1 ].ptr;

            // clear out unused positions.

            parent->nd->d[ i ].key   = 0;
            parent->nd->d[ i ].data  = 0;
            parent->nd->d[ i+1 ].ptr = 0;

            // if parent is root and size hits zero,
            // change root pointer in bti, and decrease depth

            parent->nd->decr_numitems();
            if ( parent->nd->get_numitems() == 0 )
            {
                if ( !parent->nd->is_root() )
                {
                    // should not happen, if it does, debug me!
                    printf( "OBtree::delete : error! nonroot node shrunk!\n" );
                    ret = DELETE_FAIL;
                }
                oldrootblockno = ndstptr->noderecno;
                bti->rootblockno = curnod_blockno;
                bti->depth--;
                bti->numnodes--;
                r->nd->set_root();
                oldrootfree = true;
            }

            parent->dirty = true;
            curnod->dirty = true;

            // delete sib

            switch ( whichsib_coalesce )
            {
            case SIB_LEFT:
                unlock_node( l );
                fbn_nodes->free( l_sibblockno );
#ifdef DEBUG
                printf( "freed left %d\n", l_sibblockno );
#endif
                l_sibling = NULL;
                break;

            case SIB_RIGHT:
                // can't delete l, because l is curnod.
                // instead, copy contents over and rename.
                memcpy( curnod->nd, r->nd, node_size( bti->order ));
                unlock_node( r );
                fbn_nodes->free( r_sibblockno );
#ifdef DEBUG
                printf( "freed right %d\n", r_sibblockno );
#endif
                r_sibling = NULL;
                // and update parent to point to this node correctly.
                parent->nd->d[ parentindex ].ptr = curnod_blockno;
                break;

            case SIB_NONE:
                // nothing
                break;
            }

            bti->numnodes--;
        }

        if ( l_sibling )
            unlock_node( l_sibling );
        if ( r_sibling )
            unlock_node( r_sibling );
    }

    while ( 1 )
    {
        curnod = get_nodstor( curnod_blockno, index );
        if ( curnod == NULL )
            break;
        unlock_node( curnod );
    }

    if ( oldrootfree )
    {
        fbn_nodes->free( oldrootblockno );
#ifdef DEBUG
        printf( "freed root %d\n", oldrootblockno );
#endif
    }

    bti->numrecords--;

    return ret;
}
