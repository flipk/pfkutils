
/** \file BtreeNode.C
 * \brief implementation of BTNode and BTNodeCache objects
 * \author Phillip F Knaack
 */

#include "Btree.H"
#include "Btree_internal.H"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>

BTNode :: BTNode( FileBlockInterface * _fbi, int _btorder, UINT32 _fbn )
{
    fbi = _fbi;
    btorder = _btorder;
    fbn = _fbn;
    refcount = 0;

    ptrs = new UINT32[ btorder ];
    datas = new UINT32[ btorder-1 ];
    keys = new BTKey*[ btorder-1 ];

    int i;
    for (i=0; i < (btorder-1); i++)
        keys[i] = NULL;

    if (fbn == 0)
    {
        numitems = 0;
        leaf = true;
        root = true;

        for (i=0; i < btorder; i++)
            ptrs[i] = 0;
        for (i=0; i < (btorder-1); i++)
            datas[i] = 0;

        // do a dummy alloc, to get a valid fbn,
        // so that BTNodeCache can insert us into
        // the hash right after the constructor.
        fbn = fbi->alloc(8);

        // force this to be written 
        dirty = true;
    }
    else
    {
        BTNodeDisk  btn(fbi);
        btn.get( fbn );
        numitems = btn.d->get_numitems();
        leaf = btn.d->is_leaf();
        root = btn.d->is_root();

        // get ptrs
        for (i=0; i < (numitems+1); i++)
            ptrs[i] = btn.d->items[i].ptr.get();

        UCHAR * keydata = btn.d->get_key_data(btorder);

        // get keys, datas, alloc key memory, extract key data
        for (i=0; i < numitems; i++)
        {
            datas[i] = btn.d->items[i].data.get();
            UINT32 keylen = btn.d->items[i].keysize.get();
            keys[i] = new(keylen) BTKey(keylen);
            memcpy( keys[i]->data,
                    keydata + btn.d->items[i].keystart.get(),
                    keys[i]->keylen );
        }

        dirty = false;
    }
}

BTNode :: ~BTNode( void )
{
    if (dirty)
        store();
    int i;
    for (i=0; i < (btorder-1); i++)
        if (keys[i])
            delete keys[i];
    delete[] ptrs;
    delete[] datas;
    delete[] keys;
}

void
BTNode :: store( void )
{
    int size, i;

    if (dirty == false)
        return;

    // first calculate the amount of space needed for the
    // node plus keys.

    size = _BTNodeDisk::node_size(btorder);

    for (i=0; i < numitems; i++)
        size += keys[i]->keylen;

    // realloc the extent
    fbi->realloc( fbn, size );

    // begin building the node.
    BTNodeDisk  btn(fbi);
    btn.get( fbn, true );
    btn.d->magic.set( _BTNodeDisk::MAGIC );
    btn.d->set_numitems( numitems );
    btn.d->set_root( root );
    btn.d->set_leaf( leaf );

    UCHAR * keydata = btn.d->get_key_data(btorder);
    int keystart = 0;

    for (i=0; i < numitems; i++)
    {
        btn.d->items[i].ptr.set( ptrs[i] );
        btn.d->items[i].keystart.set( keystart );
        btn.d->items[i].keysize.set( keys[i]->keylen );
        btn.d->items[i].data.set( datas[i] );
        memcpy( keydata + keystart,
                keys[i]->data,
                keys[i]->keylen );
        keystart += keys[i]->keylen;
    }
    if (i > 0)
        btn.d->items[i].ptr.set( ptrs[i] );

    dirty = false;
}

BTNodeCache :: BTNodeCache( FileBlockInterface * _fbi,
                            int _btorder,
                            int _max_nodes )
{
    fbi = _fbi;
    btorder = _btorder;
    max_nodes = _max_nodes;
}

BTNodeCache :: ~BTNodeCache( void )
{
    BTNode * n;
    while ((n = lru.dequeue_head()) != NULL)
    {
        hash.remove(n);
        delete n;
    }
}

BTNode *
BTNodeCache :: get( UINT32 fbn )
{
    BTNode * n;

    if (fbn != 0)
    {
        n = hash.find(fbn);
        if (n)
        {
            if (n->refcount++ == 0)
                lru.remove(n);
            return n;
        }
    }

    // trim the cache.
    while (lru.get_cnt() > max_nodes)
    {
        n = lru.dequeue_head();
        hash.remove(n);
        delete n;
    }

    n = new BTNode( fbi, btorder, fbn );
    hash.add(n);
    n->refcount++;
    return n;
}

void
BTNodeCache :: release( BTNode * n )
{
    if (--n->refcount == 0)
        lru.add(n);
}
