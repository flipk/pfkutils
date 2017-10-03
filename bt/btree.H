
#ifndef __BTREE_H__
#define __BTREE_H__

#include <stdarg.h>

#include "recnoblock.H"

//
// btree
//
// to put a record:
//    alloc_rec to create a rec. the returned rec
//      contains locked block pointers.
//    put_rec which inserts the rec into the btree
//      and then unlocks the blocks and frees the rec pointer.
//
// to get a record:
//    call get_rec with the key, get the returned rec.
//    when done with the rec pointer, call unlock_rec.
//    you may not modify the key portion. instead, call alloc_rec
//      and create a brand new entry added with put_rec.
//    you may modify the data portion of the returned rec,
//      as long as you call unlock_rec with a "true" dirty flag
//      when you are done.
//    if you want to change the size of the data portion then
//      you must delete the record and create a new one. <-- xxx
//
// to delete a record
//    call delete_rec.
//
//
//  TODO:  fix the "xxx" above.
//
// note also that with "get_fbn" public you are allowed to store
// other data in this file as well, but you are required to do your
// own maintenence on the block numbers. you may for instance chain
// a linked list off of a btree record or something like that.
//

class btree_printinfo {
public:
    static const int NODE_INFO       = 1;
    static const int BTREE_INFO      = 2;
    static const int KEY_REC_PTR     = 4;
    static const int DATA_REC_PTR    = 8;
    static const int _FIRST_USER_OPT = 16;
    int options;
    btree_printinfo( int _options ) { options = _options; }
    virtual ~btree_printinfo( void ) { /*placeholder*/ }
    // sprintelement should return null if dumptree/dumpnode should stop.
    virtual char * sprint_element( int noderec,
                                   int keyrec, void * key, int keylen,
                                   int datrec, void * dat, int datlen,
                                   bool * datdirty ) = 0;
    // dumpnode will call sprintelementfree when its done
    // actually doing the printing.
    virtual void sprint_element_free( char * s ) = 0;
    // this is the function that actually prints.
    virtual void print( char * format, ... )
        __attribute__ ((format( printf, 2, 3 ))) = 0;

};

class Btree {
public:
    enum put_retval {
        PUT_NEW = 1,
        PUT_OVERWRITE = 2,
        PUT_FAIL = 99
    };
    enum delete_retval {
        DELETE_OK = 1,
        DELETE_KEY_NOT_FOUND = 2,  // arg is not freed in this case
        DELETE_FAIL = 99
    };

    struct field {
        UCHAR *ptr;
        UINT32 recno;
        UINT32 magic;
        int len;
        bool dirty;
    };
    struct rec {
        field key;
        field data;
    };

private:
    static const UINT32 BTI_MAGIC = 0x0d83f386;
    static const UINT32 NODE_MAGIC = 0x4463ab2c;
    static const UINT32 INVALID_BLK = 0xFFFFFFFF;
    static const UINT32 MAX_BTI_HUNT = 8192;

    int BTREE_ORDER;
    int HALF_ORDER;
    int ORDER_MO;

    struct _node {
        // this is what a node on disk looks like.
        // node_magic signature
        UINT32 magic;
    private:
        int numitems;
        enum constants {
            LEAF_NODE = 0x8000,
            ROOT_NODE = 0x4000,
            NUM_MASK  = 0x3fff
        };
    public:
        struct node_data {
            UINT32 ptr;  // node pointer
            UINT32 key;  // recno for key block
            UINT32 data; // recno for data block
        } d[ 0 ];
        int get_numitems(void)   { return numitems & NUM_MASK; }
        void set_numitems(int v) {
            numitems = (numitems & ~NUM_MASK) + (v & NUM_MASK);
        }
        void incr_numitems(void) { set_numitems( get_numitems() + 1 ); }
        void decr_numitems(void) { set_numitems( get_numitems() - 1 ); }
        bool is_leaf(void)       { return ((numitems & LEAF_NODE) != 0); }
        bool is_root(void)       { return ((numitems & ROOT_NODE) != 0); }
        void set_leaf(void)      { numitems |=  LEAF_NODE; }
        void clear_leaf(void)    { numitems &= ~LEAF_NODE; }
        void set_root(void)      { numitems |=  ROOT_NODE; }
        void clear_root(void)    { numitems &= ~ROOT_NODE; }
    };
    static int node_size( int order ) {
        return  sizeof(_node) + sizeof(int) + 
            ((order-1) * sizeof(_node::node_data));
    }
    struct btreeinfo {
        // this is the btree info block on disk.
        UINT32 magic;
        UINT32 bti_recno;
        UINT32 rootblockno;
        int numnodes;
        int numrecords;
        int depth;
        int order;
    };
    struct node {
        _node * nd;
        UINT32 magic;
        bool dirty;
    };

    btreeinfo * bti;
    UINT32 bti_magic;

    // compare 
    int compare_recs( rec *, rec * );
    int walk_node( node *, rec *, bool &exact );

    // these things are used by various algorithms needing
    // to store lists of nodes and indices.
    struct nodstor {
        nodstor * next;
        node * n;
        UINT32 noderecno;
        int index;
    };
    nodstor * nods;
    void add_nodstor( node * n, UINT32 noderecno, int index );
    node * get_nodstor( UINT32 &noderecno, int &index );

    // split a node into two at selected index; include record.
    // return blockno for new rightnode. used by put()
    int    split_node( node *, rec *, UINT32 rightnode, int index );

    node * fetch_node( UINT32 blockno )
        { return fetch_node( blockno, false ); }
    node * fetch_node( UINT32 blockno, bool newnode );
    void   unlock_node( node * );

    rec  * fetch_rec( UINT32 keyblockno, UINT32 datablockno );

    FileBlockNumber * fbn;
public:
    class constructor_failed { 
    public:
        enum failure_reason {
            NO_ERROR,
            NO_NODES_FOUND
        } reason;
        constructor_failed( failure_reason x ) { reason = x; }
    };
    FileBlockNumber * get_fbn(void) { return fbn; }

    static void new_file( FileBlockNumber * fbn, int order );

    Btree( FileBlockNumber * ) throw ( constructor_failed );
    ~Btree( void );

    int flush( void ) {
        UINT32 bn = bti->bti_recno;
        fbn->unlock_block( bti_magic, true );
        int ret = fbn->flush();
        bti = (btreeinfo*) fbn->get_block( bn, &bti_magic );
        return ret;
    }


    void dumptree( btree_printinfo * );
private:
    bool dumpnode( btree_printinfo *, int recno ); // return false to stop

public:
    void  unlock_rec( rec * );

    rec * alloc_rec( int keylen, int datalen );
    rec * get_rec( UCHAR *keyptr, int keylen );

    put_retval put_rec( rec * );

    delete_retval delete_rec( UCHAR * keyptr, int keylen );
    delete_retval delete_rec( rec * );
};

#endif /* __BTREE_H__ */
