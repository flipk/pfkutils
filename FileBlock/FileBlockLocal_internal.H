#if 0

#include "dll2.H"

#define FILE_BLOCK_SIGNATURE  0x43206810

enum { EXTENT_LIST, NUM_LISTS };

class Extent {
public:
    LListLinks <Extent> links[ NUM_LISTS ];
    off_t offset;
    
#endif
