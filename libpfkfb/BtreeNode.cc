
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

/** \file BtreeNode.cc
 * \brief implementation of BTNode and BTNodeCache objects
 */

#include "Btree.h"
#include "Btree_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>

BTNode :: BTNode( FileBlockInterface * _fbi, int _btorder, FB_AUID_T _fbn )
{
    fbi = _fbi;
    btorder = _btorder;
    fbn = _fbn;
    refcount = 0;

    ptrs = new FB_AUID_T[ btorder ];
    datas = new FB_AUID_T[ btorder-1 ];
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

        uint8_t * keydata = btn.d->get_key_data(btorder);
        int keystart = 0;

        // get keys, datas, alloc key memory, extract key data
        for (i=0; i < numitems; i++)
        {
            datas[i] = btn.d->items[i].data.get();
            uint32_t keylen = btn.d->items[i].keysize.get();
            keys[i] = new(keylen) BTKey(keylen);
            memcpy( keys[i]->data, keydata + keystart,
                    keys[i]->keylen );
            keystart += keys[i]->keylen;
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

    uint8_t * keydata = btn.d->get_key_data(btorder);
    int keystart = 0;

    for (i=0; i < numitems; i++)
    {
        btn.d->items[i].ptr.set( ptrs[i] );
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
BTNodeCache :: get( FB_AUID_T fbn )
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
        lru.add_head(n);
}

void
BTNodeCache :: delete_node( BTNode * n )
{
    if (n->refcount != 1)
    {
        fprintf(stderr, "ERROR: delete_node called on node "
                "with refcount %d\n", n->refcount);
        kill(0,6);
    }

    hash.remove(n);
    FB_AUID_T fbn = n->get_fbn();

    // clear the node's keys to prevent it
    // deleting all memory associated with those
    // keys; if the node is being deleted, the
    // keys were most assuredly moved to another
    // node and their memory is still valid.
    int i;
    for (i=0; i < (btorder-1); i++)
        n->keys[i] = NULL;

    // clear the node's dirty flag to prevent
    // an unnecessary store() invocation during
    // the delete.
    n->dirty = false;
    delete n;

    // free the space in the file consumed by the node.
    fbi->free(fbn);
}
