
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
typedef FileBlockT <_BTInfo> BTInfo;

/** \todo optimize nodes and performance by putting key data
 * inline with the nodes?   investigate. */
struct BTNodeItem {
    UINT32_t ptr;   /**< fileblock number of a left-child node */
    UINT32_t key;   /**< fileblock number of the key */
    UINT32_t data;  /**< fileblock number of the data */
};

/** \note this structure is variable-sized. */
struct _BTNode {
    static const UINT32 MAGIC = 0x4463ab2d;
    UINT32_t    magic;
    UINT32_t    _numitems;  /**< count of items, also encode leaf and root */
    enum ITEM_CONSTANTS {
        LEAF_MASK = 0x8000,
        ROOT_MASK = 0x4000,
        NUM_MASK  = 0x3fff
    };
    /** \note must be last item */
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
        return sizeof(_BTNode) +
            sizeof(BTNodeItem)*(_order-1) +
            sizeof(UINT32_t);
    }
};
typedef FileBlockT <_BTNode> BTNode;

class BtreeInternal : public Btree {
    int BTREE_ORDER;
    int HALF_ORDER;
    int ORDER_MO;   /**< order minus one */
    int node_size;
    UINT32 info_fbn;
    BTInfo info;
    static int compare_items( _BTDatum * one, _BTDatum * two );
    /** return false if printing should stop */
    bool printnode( BtreePrintinfo * pi, UINT32 node_fbn );
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
