
enum { FBL_LIST, FBL_LRU, FBL_HASH, FBL_NUM_LISTS };

// FileBlockLocalBlock
class FBLB : public FileBlockCookie {
    static const int MAGIC = 0x8f94dc9;
    int magic;
public:
    FBLB(UINT32 _block, off_t _offset, int _size) {
        magic = MAGIC;
        block = _block;
        offset = _offset;
        size = _size;
        ptr = new UCHAR[size];
        dirty = false;
    }
    ~FBLB(void) {
        delete[] ptr;
    }
    bool valid(void) { return (magic == MAGIC); }
    LListLinks<FBLB> links[FBL_NUM_LISTS];
    UINT32 block;
    off_t offset;
    int size;
    int fd;
    bool dirty;
    UCHAR * ptr;
};

class FileBlockLocalHashComparator {
public:
    static int hash_key( FBLB * item ) { return item->block; }
    static int hash_key( UINT32 key ) { return (int)(key & 0x7FFFFFFF); }
    static int hash_key_compare( FBLB * item, UINT32 key ) {
        return (item->block == key);
    }
};

// this is a DLL2 compound container class.
// an item is always on the hash and list.
// however it is not on the lru if it is in the 'locked' state.
class FileBlockLocalCache {
    int fd;
    int total_size;
    int max_size;
    LList     <FBLB,FBL_LIST> list;
    LListLRU  <FBLB,FBL_LRU > lru;
    LListHash <FBLB,UINT32,
               FileBlockLocalHashComparator,FBL_HASH> hash;
    void remove( FBLB * b );
    void trim( void );
public:
    FileBlockLocalCache(int _fd, int _max_size);

    // flushes all cached blocks.  NOTE this does not close fd!
    ~FileBlockLocalCache(void);

    int get_cnt( void ) { return list.get_cnt(); }
    int get_total_size( void ) { return total_size; }

    // search for the block in the cache.
    // if we don't have it in cache, return NULL.
    // if we do have it, lock it and return.
    // if it is already locked, crash.
    FBLB * find( UINT32 block );

    // get a block for write.  this does NOT read from the
    // file, instead returns a fresh zero'd buffer.  NOTE
    // this assumes the block does not already exist in the 
    // cache, and does not check!
    FBLB * get_for_write( UINT32 block, off_t offset, int size );

    // get the block from the file.  NOTE this assumes
    // the block does not already exist in the cache,
    // and does not check!
    FBLB * get( UINT32 block, off_t offset, int size );

    // if the block is already locked, this will crash!
    void unlock( FBLB * b, bool dirty );
    void flush( void );
};
