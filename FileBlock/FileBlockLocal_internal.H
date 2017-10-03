
enum { FBL_LIST, FBL_LRU, FBL_HASH, FBL_NUM_LISTS };

// FileBlockLocalBlock
class FBLB : public FileBlockCookie {
    static const UINT32 MAGIC = 0x8f94dc9;
    UINT32 magic;
public:
    FBLB(UINT32 _block, off_t _offset, int _size) {
        magic = MAGIC;
        block = _block;
        offset = _offset;
        size = _size;
        user = false;
        dirty = false;
        ptr = new UCHAR[size];
    }
    ~FBLB(void) {
        magic = 0;
        delete[] ptr;
    }
    bool valid(void) { return (magic == MAGIC); }
    LListLinks<FBLB> links[FBL_NUM_LISTS];
    UINT32 block;
    UINT32 user:1;
    UINT32 dirty:1;
    UINT32 size:30;
    off_t offset;
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

/*

each 'block' in a data file is identified by a UINT32.  any UINT32
value is valid except for 0 and ffffffff.  block 1 is reserved for use
by the api, for storage of data info blocks.

block ids are allocated using 'random'.  block ids do not correspond
directly to a position in the file.  instead there is a mapping table,
where a block id is translated to a position in the file.

the block mapping table is a large hash table.




a 'segment' is an 8MB portion of a file.  the first 64KB of each
segment is a free-space bitmap.  (There are 524288 bits in a 64KB
page.)  Each bit represents a 16-byte unit.



 */
