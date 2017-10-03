/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

/** \file Btree_internal.H
 * \brief declaration of nitty gritty guts of Btree / BtreeInternal objects.
 * \author Phillip F Knaack
 */

#include "dll2.H"

class BtreeInternal;

/** a Btree information block as it appears on disk. */
struct _BTInfo {
    static const UINT32 MAGIC = 0x0d83f387;
    UINT32_t magic;
    UINT32_t bti_fbn;     /**< fileblock number of this info block (self) */
    UINT32_t root_fbn;    /**< fileblock number of the root node */
    UINT32_t numnodes;    /**< count of nodes in file */
    UINT32_t numrecords;  /**< count of records in file */
    UINT32_t depth;       /**< depth of the btree */
    UINT32_t order;       /**< order number of the btree */
};
/** a FileBlock shortcut for accessing Btree information blocks */
typedef FileBlockT <_BTInfo> BTInfo;

/** an item within a _BTNodeDisk */
struct BTNodeItem {
    UINT32_t ptr;       /**< fileblock number of a left-child node */
    UINT16_t keysize;   /**< length of key data */
    UINT32_t data;      /**< fileblock number of the data */
};

/** a Btree node as it appears on disk, plus access methods.
 * \note this structure is variable-sized. */
struct _BTNodeDisk {
    static const UINT32 MAGIC = 0x4463ab2d;
    UINT32_t    magic;      /**< must equal _BTNodeDisk::MAGIC */
private:
    UINT16_t    _numitems;  /**< count of items, also encode leaf and root
                               using _BTNodeDisk::ITEM_CONSTANTS enum */
    /** bitfields of _BTNodeDisk::_numitems */
    enum ITEM_CONSTANTS {
        LEAF_MASK = 0x8000,  /**< this node is a leaf */
        ROOT_MASK = 0x4000,  /**< this node is the root node */
        NUM_MASK  = 0x3fff   /**< remaining bits for the number of items */
    };
public:
    /** the 'items' in this node (keys, data, and node pointers).
     * This member is zero-length because it is the start of a variable-
     * length array.  The dimension of the array depends on the order of
     * the btree: the dimension is order-1.
     * \note Must be last item.
     * \note A btree of order N has N-1 items, but N pointers. 
     *       Thus There is an additional 'ptr' after this.
     */
    BTNodeItem  items[ 0 ];
    int get_numitems(void) { return _numitems.get() & NUM_MASK; }
    bool is_root(void) { return (_numitems.get() & ROOT_MASK) == ROOT_MASK; }
    bool is_leaf(void) { return (_numitems.get() & LEAF_MASK) == LEAF_MASK; }
    void set_numitems(int v) {
        _numitems.set( (_numitems.get() & ~NUM_MASK) + (v & NUM_MASK) );
    }
    int inc_numitems(void) {
        int v = get_numitems() + 1;
        set_numitems(v);
        return v;
    }
    int dec_numitems(void) {
        int v = get_numitems() - 1;
        set_numitems(v);
        return v;
    }
    void set_root(bool v) {
        if (v) _numitems.set( _numitems.get() |  ROOT_MASK );
        else   _numitems.set( _numitems.get() & ~ROOT_MASK );
    }
    void set_leaf(bool v) {
        if (v) _numitems.set( _numitems.get() |  LEAF_MASK );
        else   _numitems.set( _numitems.get() & ~LEAF_MASK );
    }
    static int node_size( int _order ) {
        // add in order-1 items plus one more ptr.
        return sizeof(_BTNodeDisk) +
            sizeof(BTNodeItem)*(_order-1) +
            sizeof(UINT32_t);
    }
    UCHAR * get_key_data(int _order) {
        return ((UCHAR*)this) + node_size(_order);
    }
};
typedef FileBlockT <_BTNodeDisk> BTNodeDisk;

/** generic data structure for comparing keys etc.
 * this is a variable-sized structure, so the syntax to construct it
 * is a little different:
 * <pre>
 *    BTKey * key = new(keylen) BTKey(keylen);
 * </pre>
 * The "keylen" parameter must be specified both to the operator and to
 * the constructor so that the right amount of memory may be allocated.
 */
struct BTKey {
    UINT32 keylen;     /**< the length of the key data */
    UCHAR data[0];     /**< the key data itself */
    //
    /** constructor will populate the keylen field from its parameter */
    BTKey( UINT32 _keylen ) { keylen = _keylen; }
    /** operator allocates memory based on size of key.
     * \param sz  the compiler provides this as sizeof(BTKey) however this 
     *            parameter is not used by this method.
     * \param keylen the length of the key data.
     * \return enough memory for BTKey and the key data combined. */
    void * operator new( size_t sz, int keylen ) {
        return (void*) new UCHAR[ sizeof(BTKey) + keylen ];
    }
    /** operator which matches the custom new operator to free memory. */
    void operator delete( void * ptr ) {
        delete[] (UCHAR*)ptr;
    }
};

/** an enum for the LList declarations for BTNode cache */
enum BTNodeLists { 
    BTNODE_LRU,      /**< unused BTNodes are on a least-recently used list */
    BTNODE_HASH,     /**< all BTNodes are on a hash by fbn */
    BTNODE_NUMLISTS  /**< dimension of the links array */
};

/** a type for managing nodes.  if you haven't noticed, _BTNodeDisk is
 * a little clumsy to use, especially considering the key-data is stored
 * contiguously after the items-- and items is variable-size depending
 * on the btree's order-- and of course don't forget each key is
 * variable-sized also.  this structure simplifies all the accesses
 * to the node. */
class BTNode {
    friend class BTNodeHashComparator;
    friend class BTNodeCache;
    FileBlockInterface * fbi;   /**< how to access the disk file. */
    int btorder;                /**< the order of the btree, used to calc
                                   the size of the variable-sized portions
                                   of the node on disk. */
    UINT32 fbn;                 /**< FileBlock ID of the node on disk */
    int refcount;               /**< BTNodeCache keeps track of # users */
    bool dirty;
public:
    /** BTNodeCache keeps BTNodes in an LRU and a Hash */
    LListLinks <BTNode> links[BTNODE_NUMLISTS];
    // let the public (BtreeInternal) access these fields.
    int numitems;    /**< number of items in this node */
    bool leaf;       /**< this is a leaf node */
    bool root;       /**< this is a root node */
    UINT32 * ptrs;   /**< array of FileBlock IDs of ptrs to nodes */
    UINT32 * datas;  /**< array of FileBlock IDs of ptrs to data */
    BTKey ** keys;   /**< array of keys */
    /** constructor for a node. 
     * \param _fbi  the FileBlockInterface to get the node in and out
     * \param _btorder  the Btree order, so that the size of a node can be
     *                  calculated.
     * \param _fbn  the FileBlock ID of the node; if this is specified as
     *              zero, a new block will be allocated on a store.
     */
    BTNode( FileBlockInterface * _fbi, int _btorder, UINT32 _fbn );
    /** destructor frees memory consumed by the node */
    ~BTNode( void );
    /** return the fbn. */
    UINT32 get_fbn(void) { return fbn; }
    /** mark this node as modified */
    void mark_dirty(void) { dirty = true; }
    /** realloc the fbn and store the node data into the file. */
    void store( void );
};

/** a DLL2 helper to help sort BTNode objects into the BTNodeCache hash. */
class BTNodeHashComparator {
public:
    /** how to access a BTNode and return a hash index */
    static int hash_key( BTNode * item ) {
        return item->fbn & 0x7FFFFFFF;
    }
    /** how to access a key (fbn) and return a hash index */
    static int hash_key( UINT32 key ) {
        return key & 0x7FFFFFFF;
    }
    /** how to compare a key (fbn) to an item (BTNode) */
    static bool hash_key_compare( BTNode * item, UINT32 key ) {
        return (item->fbn == key);
    }
};

/** typedef for a least-recently used list of BTNode */
typedef LListLRU <BTNode, BTNODE_LRU>  BTNodeLRU;
/** typedef for a hash on fbn of BTNode */
typedef LListHash <BTNode, UINT32,
                   BTNodeHashComparator, BTNODE_HASH>   BTNodeHash;

/** a cache of BTNode objects, with least-recently used list, cap on
 * max number of nodes, and a hash searchable by fbn.  All accesses to
 * read and write nodes should go through this interface. */
class BTNodeCache {
    BTNodeLRU   lru;           /**< the least-recently-used list of BTNode */
    BTNodeHash  hash;          /**< a hash on fbn of BTNode */
    FileBlockInterface * fbi;  /**< the file to access for nodes */
    int btorder;               /**< the order of the btree, to assist in 
                                * calculating node sizes */
    int max_nodes;             /**< a cap on the max size of the LRU */
public:
    /** constructor for the cache.
     * \param _fbi the file to access for nodes
     * \param _btorder the order of the btree
     * \param _max_nodes the maximum number of nodes to keep in the LRU cache */
    BTNodeCache( FileBlockInterface * _fbi, int _btorder, int _max_nodes );
    /** destructor frees all memory used by all nodes */
    ~BTNodeCache( void );
    /** retrieve a node either from the cache or from disk if not found
     * in the cache.
     * \param fbn  the FileBlock ID number of the node.  This may be specified
     *             as zero for a brand new node.  the ID will be allocated. 
     *             you will need to fetch the ID using BTNode::get_fbn.
     * \return the node, either from cache or a new one just read and parsed
     *         from the FileBlockInterface.
     * \note This method returns a node with a nonzero reference count.  When
     *       done with the node, do NOT delete it!
     *       instead call BTNodeCache::release. */
    BTNode * get( UINT32 fbn );
    /** return a node to the cache.
     * \param n the node to return
     * \note This method decreases the node's reference count.  If the refcount
     *       reaches zero, the node is put back in the LRU and is eligible for
     *       flushing if it eventually hits the end of the LRU. */
    void release( BTNode * n );
    /** allocate a new node.  This will be called when the tree has been 
     * filled to the point where the height must increase and a new root node
     * must be created.  Just as with the return of BTNodeCache::get method,
     * you must eventually call BTNodeCache::release on this ptr.
     * \return the new BTNode object. */
    BTNode * new_node(void) { return get(0); }
    /** delete a node no longer in use.
     * Do not call until all ptrs and data are removed to another place.
     * This will be called when the tree must be reduced by a level and the
     * root node freed and replaced.  It is assumed the argument to this
     * function was originally obtained thru BTNodeCache::get.
     * \param n the node to delete. */
    void delete_node( BTNode * n );
};

/** internal implementation of Btree.
 * the existence of this class hides a lot of internal implementation
 * details from the user, who doesn't care to have his namespace
 * polluted with all the other data types we have to pull in to make
 * this work.
 */
class BtreeInternal : public Btree {
    /** maximum number of nodes to hold in the cache.
     * \note this number needs fiddling. */
    static const int MAX_NODES = 2000; 
    /** the order of the tree (maximum number of pointers
     * at each node; must be an odd number <= MAX_ORDER */
    int BTREE_ORDER;
    /** half of the order (rounded down) */
    int HALF_ORDER;
    /** order minus one, used frequently enough to require its
     * own constant */
    int ORDER_MO;
    /** the size in bytes of a node, depends on order; this is
     * a constant for a given order, but used often enough to benefit
     * from calculating once during init and storing */
    int node_size;
    /** the FileBlock ID of the BTInfo block in the file */
    UINT32 info_fbn;
    /** a pointer to the FileBlock containing the BTInfo; this 
     * object is always live. */
    BTInfo info;
    /** the cache of all nodes currently in use. */
    BTNodeCache  * node_cache;
    /** compare two btree keys, and return -1, 0, or 1 depending on
     * what order they should be placed in. the first key is in pointer/len
     * form, to handle arguments to get/put/del, while the second is in
     * BTKey form to handle members of a node.
     * \param key pointer to the first item's key data.
     * \param keylen length of the first item's key data.
     * \param two the second key to compare
     * \return -1 if one is "less than" two, 0 if one is binary equivalent
     *         to two, 1 if one is "greator than" two.
     */
    static int compare_keys( UCHAR * key, int keylen, BTKey * two );
    /** compare key to each key in node.  key is in pointer/len form to 
     * handle arguments to get/put/del.
     * if exact match is found, *exact is set to true.
     * \param n the node to walk through
     * \param key pointer to key data
     * \param keylen length of the key data
     * \param exact a pointer to a bool, which this function will set to
     *              true or false prior to returning to indicate if the key
     *              matches a key in the node exactly or not.
     * \return  index into node's items array; <ul> <li> if 'key' were
     *  inserted into node, this would be the index where it should go.
     *  <li> if this is a non-leaf, the pointer at that index should be
     * followed to get closer to the desired item. </ul>
     */
    int walknode( BTNode * n, UCHAR * key, int keylen, bool *exact );
    /** take full node plus 1 item and split into 2 nodes
     * plus 1 "pivot" item between them.  Supply a key and an associated
     * data, plus a node pointer to the right of that key, and an index
     * into the existing node where this key goes; the existing node becomes
     * the new left-node; creates a new right-node; and returns the key+data
     * which is the pivot record between the left-node and the right-node;
     * and returns the block ID of the new rightnode.
     * \note that the returned *key and *data_fbn may or may not be the same
     *       as when passed in; this depends on whether the passed-in key
     *       happens to lie in exactly the middle of the original node or not.
     * \note this function only works properly if node n is full!
     * \param n the full BTNode
     * \param key pointer to a pointer to the key of the item to insert;
     *            upon return, *key will be updated to point to the pivot
     *            key which separates the two nodes.
     * \param data_fbn a ptr to a block ID for the data of the item to insert;
     *                 upon return, *data_fbn will be updated to point to
     *                 the data assocated with the pivot key.
     * \param rightnode a FileBlock ID of the node to the right of the item
     * \param idx the index into node n where this item goes (from walknode)
     * \return the FileBlock ID of the new right node that was created.
     */
    UINT32 splitnode( BTNode * n, BTKey ** key, UINT32 * data_fbn,
                   UINT32 rightnode, int idx );
    /** walk thru all the data in a node, recursing on the pointers, and
     * calling a user-specified function for each key and data (item).
     * \param bti the user's BtreeIterator object.
     * \param node_fbn the FileBlock ID of the node to walk.
     * \return true if the entire node was handled uninterrupted, or false 
     *         if BtreePrintinfo returned false, indicating that
     *         the entire tree-walk should be terminated.
     */
    bool iterate_node( BtreeIterator * bti, UINT32 node_fbn );
    bool iterate_inprogress;    /**< cannot modify during an iterate method */
public:
    /** the FileBlockInterface::get_data_info_block string name
     * for the info field which describes the Btree. */
    static const char BTInfoFileInfoName[];
    static const int MAX_ORDER = 51;    /**< maximum order supported */
    /** constructor of BtreeInternal class which takes the FileBlockInterface
     * as an argument.  This function attempts to locate and read the BTInfo
     * structure from the file and validate it.  It also learns the Btree order
     * and precalculates some constants.
     * \param _fbi  the interface to the file containing the btree
     */
    BtreeInternal( FileBlockInterface * _fbi );
    /** check if a file is a valid btree file.
     * this function checks a number of things: first that it can locate the
     * BTInfo block, then that the _BTInfo::MAGIC matches, then that the order
     * is sensible, then that the bti_fbn number matches.  If all these tests
     * pass, then the likelihood that this is a valid Btree file is pretty
     * high, so it returns true.
     * \see Btree::valid_file
     * \param _fbi the FileBlockInterface to access the file.
     * \return true if the file appears to be a valid btree file,
     *              false if not. */
    static bool valid_file( FileBlockInterface * _fbi );
    /** free all memory associated with the btree and sync all nodes
     * back to the file. */
    /*virtual*/ ~BtreeInternal( void );
    /*virtual*/ bool get( UCHAR * key, int keylen, UINT32 * data_id );
    /*virtual*/ bool put( UCHAR * key, int keylen, UINT32 data_id,
                          bool replace );
    /*virtual*/ bool del( UCHAR * key, int keylen );
    /*virtual*/ bool iterate( BtreeIterator * bti );
};
