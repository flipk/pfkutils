
/** \file Btree_internal.H
 * \brief declaration of nitty gritty guts of Btree / BtreeInternal objects.
 * \author Phillip F Knaack
 */

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

/** an item within a _BTNodeDisk.
 * \todo optimize nodes and performance by putting key data
 * inline with the nodes?   investigate. */
struct BTNodeItem {
    UINT32_t ptr;   /**< fileblock number of a left-child node */
    UINT16_t keystart;   /**< offset where key data starts */
    UINT16_t keysize;    /**< length of key data */
    UINT32_t data;  /**< fileblock number of the data */
};

/** a Btree node as it appears on disk, plus access methods.
 * \note this structure is variable-sized. */
struct _BTNodeDisk {
    static const UINT32 MAGIC = 0x4463ab2d;
    UINT32_t    magic;
private:
    UINT16_t    _numitems;  /**< count of items, also encode leaf and root */
    enum ITEM_CONSTANTS {
        LEAF_MASK = 0x8000,
        ROOT_MASK = 0x4000,
        NUM_MASK  = 0x3fff
    };
public:
    /** the 'items' in this node (keys, data, and node pointers)
     * \note must be last item */
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
    int node_size( int _order ) {
        // add in order-1 items plus one more ptr.
        return sizeof(_BTNodeDisk) +
            sizeof(BTNodeItem)*(_order-1) +
            sizeof(UINT32_t);
    }
};
typedef FileBlockT <_BTNodeDisk> BTNodeDisk;

/** a type for managing nodes.  if you haven't noticed, _BTNodeDisk is
 * a little clumsy to use, especially considering the key-data is stored
 * contiguously after the items-- and items is variable-size depending
 * on the btree's order-- and of course don't forget each key is
 * variable-sized also.  this structure simplifies all the accesses
 * to the node. */
class BTNode {
    FileBlockInterface * fbi;
    int btorder;
    UINT32 * fbnp;
public:
    BTNode( FileBlockInterface * _fbi, int _btorder, UINT32 * _fbnp );
    ~BTNode( void );
    void store( void );
};

/** generic data structure for comparing keys etc.
 */
struct BTGeneric {
    UCHAR data[0];
    //
    UCHAR * get_ptr (void) { return (UCHAR*) this; }
    /** this method is useless in BTGeneric, here to satisfy the template */
    int get_size (void) { return sizeof(BTGeneric); }
    /** this method is useless in BTGeneric, here to satisfy the template */
    static int get_max_size (void) { return sizeof(BTGeneric); }
};

/** internal implementation of Btree.
 * the existence of this class hides a lot of internal implementation
 * details from the user, who doesn't care to have his namespace
 * polluted with all the other data types we have to pull in to make
 * this work.
 */
class BtreeInternal : public Btree {
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
    static int compare_items( _BTDatum * one, _BTDatum * two );
    bool printnode( BtreePrintinfo * pi, UINT32 node_fbn );
    int walknode( _BTNodeDisk * n, _BTDatum * key, bool *exact );
public:
    static const char BTInfoFileInfoName[];
    static const int MAX_ORDER = 51;    /**< maximum order supported */
    BtreeInternal( FileBlockInterface * _fbi );
    /*virtual*/ ~BtreeInternal( void );
    static bool valid_file( FileBlockInterface * _fbi );
    /*virtual*/ bool get( _BTDatum * key, _BTDatum * data );
    /*virtual*/ bool put( _BTDatum * key, _BTDatum * data );
    /*virtual*/ bool del( _BTDatum * key  );
    /*virtual*/ bool printinfo( BtreePrintinfo * pi );
};
