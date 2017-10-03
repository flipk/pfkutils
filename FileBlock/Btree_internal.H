
class BtreeInternal : public Btree {
    static const char BTInfoFileInfoName[];
    int BTREE_ORDER;
    int HALF_ORDER;
    int ORDER_MO;   /**< order minus one */
public:
    static const int MAX_ORDER = 51;    /**< maximum order supported */
    BtreeInternal( FileBlockInterface * _fbi );
    /*virtual*/ ~BtreeInternal( void );
    static bool valid_file( FileBlockInterface * _fbi );
    /*virtual*/ bool get( _BTDatum * key, _BTDatum * data );
    /*virtual*/ bool put( _BTDatum * key, _BTDatum * data );
    /*virtual*/ bool del( _BTDatum * key  );
};

struct BTInfo {
    static const UINT32 MAGIC = 0x0d83f387;
    UINT32_t magic;
    UINT32_t bti_fbn;     /**< fileblock number of this info block (self) */
    UINT32_t root_fbn;    /**< fileblock number of the root node */
    UINT32_t numnodes;    /**< count of nodes in file */
    UINT32_t numrecords;  /**< count of records in file */
    UINT32_t depth;       /**< depth of the btree */
    UINT32_t order;       /**< order number of the btree */
    // BTDatum required methods
    UCHAR * get_ptr (void) { return (UCHAR*) this; }
    int     get_size (void) { return sizeof(BTInfo); }
    static int get_max_size (void) { return sizeof(BTInfo); }
};

/** \todo optimize nodes and performance by putting key data
 * inline with the nodes?   investigate. */
struct BTNodeItem {
    UINT32_t ptr;   /**< fileblock number of a left-child node */
    UINT32_t key;   /**< fileblock number of the key */
    UINT32_t data;  /**< fileblock number of the data */
};

/** \note this structure is variable-sized. */
struct BTNode {
    static const UINT32 MAGIC = 0x4463ab2d;
    UINT32_t    magic;
    UINT32_t    numitems;  /**< count of items, also encode leaf and root */
    enum ITEM_CONSTANTS {
        LEAF_NODE = 0x8000,
        ROOT_NODE = 0x4000,
        NUM_MASK  = 0x3fff
    };
    /** \note must be last item */
    BTNodeItem  items[ BtreeInternal::MAX_ORDER ];
    //xxx more
    // BTDatum required methods
    UCHAR * get_ptr (void) { return (UCHAR*) this; }
    int     get_size (void) { return sizeof(BTInfo); /*xxx wrong--dynamic */ }
    static int get_max_size (void) { return sizeof(BTNode); }
};
