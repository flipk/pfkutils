
#ifndef __EXTENT_MAP_H__
#define __EXTENT_MAP_H__

#include "types.H"
#include "dll2.H"

enum { EXTENT_LIST, EXTENT_HASH, NUM_LISTS };

class Extent {
    friend class ExtentsPage;
    Extent(void) { /* nothing */ }
    ~Extent(void) { /* nothing */ }
public:
    LListLinks <Extent> links[ NUM_LISTS ];
    off_t offset;
    UINT32 id;  // this field is valid only when used==1
    UINT32 used : 1;
    UINT32 size : 31;
};

class ExtentHashComparator {
public:
    static int hash_key( Extent * item ) {
        return item->id;
    }
    static int hash_key( UINT32 key ) {
        return key;
    }
    static bool hash_key_compare( Extent * item, UINT32 key ) {
        return (item->id == key);
    }
};

typedef LList <Extent, EXTENT_HASH> ExtentBucketList;

class Extents {
    LList <Extent, EXTENT_LIST> list;
    // only used blocks are on the id hash.
    LListHash <Extent, UINT32, ExtentHashComparator, EXTENT_HASH> hash;
    // only free blocks are on the bucket list.
    static const int NUM_BUCKETS = 2048;
    static const int BITMAP_ARRAY_SIZE = 32;
    int size_to_bucket( UINT32 size ) {
        int bucket = (size-1) >> 5;
        if (bucket >= NUM_BUCKETS)
            bucket = NUM_BUCKETS-1;
        return bucket;
    }
    UINT32 alloc_id(void);
    ExtentBucketList  buckets[ NUM_BUCKETS ];
    UINT64 bucket_bitmap[ BITMAP_ARRAY_SIZE ]; // 0 == empty
    int find_next_bucket(int bucket) {
        UINT64 reg;
        if ((bucket & 0x3F) != 0)
        {
            reg = bucket_bitmap[bucket >> 6];
            reg >>= (bucket & 0x3F);
            while ((bucket & 0x3F) != 0)
            {
                if (reg & 1)
                    return bucket;
                bucket++;
                reg >>= 1;
            }
        }
        if (bucket == NUM_BUCKETS)
            return bucket;
        while ((reg = bucket_bitmap[bucket >> 6]) == 0)
        {
            bucket += 64;
            if (bucket == NUM_BUCKETS)
                return bucket;
        }
        while (1)
        {
            if (reg & 1)
                return bucket;
            bucket++;
            reg >>= 1;
        }
    }
    void set_bit(int bucket) {
        bucket_bitmap[bucket >> 6] |=  (1ULL << (bucket & 0x3F));
    }
    void clear_bit(int bucket) {
        bucket_bitmap[bucket >> 6] &= ~(1ULL << (bucket & 0x3F));
    }
    void add_to_bucket( Extent * e ) {
        int b = size_to_bucket(e->size);
        ExtentBucketList * bucket = &buckets[b];
        bucket->add(e);
        if (bucket->get_cnt() == 1)
            set_bit(b);
    }
    void remove_from_bucket( Extent * e ) {
        int b = size_to_bucket(e->size);
        ExtentBucketList * bucket = &buckets[b];
        bucket->remove(e);
        if (bucket->get_cnt() == 0)
            clear_bit(b);
    }
public:
    Extents(void);
    ~Extents(void);
    void print(void);
    // these two are for initial setup
    void add( off_t _offset, UINT32 _size );
    void add( off_t _offset, UINT32 _size, UINT32 _id );
    // locate a block by its ID number.
    Extent * find( UINT32 id );
    Extent * alloc( UINT32 size );
    void free( UINT32 id );
    void free( Extent * e );
    // linked list walking
    Extent * get_head(void) { return list.get_head(); }
    Extent * get_next(Extent * e) { return list.get_next(e); }
};

#endif /* __EXTENT_MAP_H__ */
