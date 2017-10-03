
#include "dll2.H"

enum { BLOCK_LIST, BLOCK_NUM_LISTS };

class BCB : public BlockCacheBlock {
public:
    BCB(off_t _offset, int _size) :
        BlockCacheBlock(_offset, _size) { }
    LListLinks <BCB> links[BLOCK_NUM_LISTS];
    
};

class BlockCacheList {
public:
    LList <BCB,BLOCK_LIST>  list;
};
