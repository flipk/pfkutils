
/** \file BlockCache_internal.H
 * \author Phillip F Knaack
 * \brief internals of BlockCache implementation */

#include "dll2.H"

/** DLL2 list index identifiers for blocks on lists */
enum BLOCK_CACHE_LIST_INDICES { BLOCK_LIST, BLOCK_NUM_LISTS };

/** A BlockCacheBlock plus linked list information */
class BCB : public BlockCacheBlock {
public:
    BCB(off_t _offset, int _size) :
        BlockCacheBlock(_offset, _size) { }
    /** linked list pointers, see BLOCK_CACHE_LIST_INDICES
     * \see BLOCK_CACHE_LIST_INDICES */
    LListLinks <BCB> links[BLOCK_NUM_LISTS];
};

/** A list of blocks */
class BlockCacheList {
public:
    /** the linked list of blocks.
     * \see BLOCK_CACHE_LIST_INDICES */
    LList <BCB,BLOCK_LIST>  list;
};
