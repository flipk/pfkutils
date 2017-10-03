/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

/** \file ExtentMap.H
 * \brief Definition of interface to Extent and Extents
 * \author Phillip F Knaack */

#ifndef __EXTENT_MAP_H__
#define __EXTENT_MAP_H__

#include "types.H"
#include "dll2.H"

/** DLL2 enum for defining the lists used by Extent objects */
enum EXTENT_MAP_LIST_INDICES {
    EXTENT_LIST,         /**< if an Extent is in the ExtentFreePool, this 
                            list is used for the ExtentFreePool list.  if
                            an Extent object is in the Extents map, this 
                            index is used for the ordered Extent list.
                            \see ExtentOrderedList */
    EXTENT_ID_HASH,      /**< an Extent representing a 'used' block is on
                            the id hash using this index.  an Extent 
                            representing a 'free' block is on the bucket
                            list using this index.  this index is not used
                            for an Extent in the ExtentFreePool.
                            \see ExtentBucketList, ExtentIDHash */
    EXTENT_NUM_LISTS     /**< dimension of the links array in Extent */
};

/** an "extent" : a position and a size.
 * 
 * An Extent is identified by an identifier (if it is describing a
 * region which is in use) and includes a size and offset.
 */
class Extent {
    friend class ExtentsPage;
    /** constructor does nothing.
     * \note constructor is private to prevent users of this API from
     * creating these on their own.  only way to get one is through 
     * Extents::alloc or Extents::find. */
    Extent(void) { /* nothing */ }
    /** destructor does nothing.
     * \note destructor is private to prevent users from destroying
     * Extent objects. */
    ~Extent(void) { /* nothing */ }
public:
    /** an Extent is on two possible linked lists, 
     * \see EXTENT_MAP_LIST_INDICES */
    LListLinks <Extent> links[ EXTENT_NUM_LISTS ];
    /** the position where this extent begins */
    off_t offset;
    /** the identifier of this Extent.
     * \note the identifier is only valid if Extent::used == 1. */
    UINT32 id;  // this field is valid only when used==1
    /** indicate if this Extent is used or free */
    UINT32 used : 1;
    /** indicate the size of this Extent */
    UINT32 size : 31;
};

/** DLL2 helper class for Extent objects in a hash table,
    where the hash key is the Identifier */
class ExtentIDHashComparator {
public:
    /** how to get a key from an item */
    static int hash_key( Extent * item ) {
        return item->id;
    }
    /** how to get a key from a Key_Type */
    static int hash_key( UINT32 key ) {
        return key;
    }
    /** how to compare a Key_Type to an item */
    static bool hash_key_compare( Extent * item, UINT32 key ) {
        return (item->id == key);
    }
};

/** DLL2 helper class for Extent objects in a hash table,
    where the hash key is the start offset */
class ExtentOffsetHashComparator {
public:
    /** how to get a key from an item */
    static int hash_key( Extent * item ) {
        return (int)((item->offset >> 5) & 0x7FFFFFFF);
    }
    /** how to get a key from a Key_Type */
    static int hash_key( off_t key ) {
        return (int)((key >> 5) & 0x7FFFFFFF);
    }
    /** how to compare a Key_Type to an item */
    static bool hash_key_compare( Extent * item, off_t key ) {
        return (item->offset == key);
    }
};

/** DLL2 btree helper class for Extent objects in the bucket list,
 * where the key is the file offset. */
class ExtentBucketComparator {
public:
    static int key_compare( Extent * item, off_t key ) {
	if (item->offset < key) return 1;
	if (item->offset > key) return -1;
	return 0;
    }
    static int key_compare( Extent * item, Extent * item2 ) {
	if (item->offset < item2->offset) return 1;
	if (item->offset > item2->offset) return -1;
	return 0;
    }
    static char * key_format( Extent * item ) {
	// not used
	return NULL;
    }
};

/** a typedef for a DLL2 list of ordered Extent objects.
 * \see EXTENT_MAP_LIST_INDICES */
typedef LList <Extent, EXTENT_LIST>            ExtentOrderedList;

/** a typedef for a DLL2 list of Extent objects used for free-bucket lists.
 * \see EXTENT_MAP_LIST_INDICES */
typedef LListBTREE <Extent, off_t,
		    ExtentBucketComparator,
		    EXTENT_ID_HASH, 25>         ExtentBucketList;

/** a typedef for a DLL2 hash on the identifier.
 * \see EXTENT_MAP_LIST_INDICES */
typedef LListHash <Extent, UINT32,
                   ExtentIDHashComparator,
                   EXTENT_ID_HASH>             ExtentIDHash;

/** manages a list of Extent objects.
 *
 * This object manages Extent objects, keeping them in order,
 * searching for ideally-sized free blocks during allocation,
 * breaking up free blocks if ideally-sized blocks are unavailable,
 * and recombining blocks when free blocks become adjacent. */
class Extents {
    /** @name Extent lists */
    // @{
    /** an ordered list of all Extent objects.
     * 
     * All Extent objects are on this list, at all times.  Also, at
     * all times, the offset+size of one member must equal the offset
     * of the following member.
     * \see EXTENT_MAP_LIST_INDICES */
    ExtentOrderedList   list;
    /** hash list of used blocks, where hash key is block id.
     *
     * If an Extent is marked as used, the identifier field will be
     * valid, and the Extent will be on this hash list.  The search
     * key for the hash is the identifier.
     * \see EXTENT_MAP_LIST_INDICES */
    ExtentIDHash idhash;

    /** count of Extent objects in the 'used' state */
    int count_used;
    /** count of Extent objects in the 'free' state */
    int count_free;
    // @}

    /** Allocate an unused unique identifier.
     * \return A new unique identifier.
     * This function generates a random number and verifies 
     * this identifier is not currently in use in the hash.
     * \note This new identifier needs to be used as soon as possible
     *  to prevent possible collision (two Extent objects with same id). */
    UINT32 alloc_id(void);

    /** implementation of both alloc and realloc 
     * \param id The identifier of the old Extent to modify.  May be specified
     *           as zero to indicate no previous Extent (alloc).
     * \param size the (new) size desired for the Extent. 
     * \return The Extent allocated.
     */
    Extent * _alloc( UINT32 id, UINT32 size );

    /** @name Bucket List
     * The bucket list is for Extent objects which are free;
     * Extent objects are grouped by their size for optimal searching
     * during an alloc operation.  Buckets are divided up by 32-byte
     * sizes; i.e. buffers from 1-31 bytes are bucket 0, buffers of size
     * 32-63 are bucket 1, etc.
     * \note The bucket-list heavily depends on the 32-byte roundup in
     *  Extents::alloc which guarantees that all Extent objects start on
     *  a 32-byte boundary.  (This algorithm uses a 5-bit shift to index
     *  the bucket array.)  Without that guarantee, the allocation algorithm
     *  may incorrectly return overlapping allocations.
     */
    // @{
    /** 2048 buckets for any size up to 65536 bytes. */
    static const int NUM_BUCKETS = 2048;
    /** 64-bit bitmaps in an array, 64*32=2048 */
    static const int BITMAP_ARRAY_SIZE = 32;
    /** convert a size value to a bucket number.
     * \note if the size is greator than the maximum bucket size,
     * the return is capped to NUM_BUCKETS-1. 
     * \param size the size of the bucket
     * \return the index into buckets array */
    int size_to_bucket( UINT32 size ) {
        int bucket = (size-1) >> 5;
        if (bucket >= NUM_BUCKETS)
            bucket = NUM_BUCKETS-1;
        return bucket;
    }
    /** the array of linked lists of Extent objects */
    ExtentBucketList  buckets[ NUM_BUCKETS ];
    /** a bitmap indicating empty bucket lists in the array.
     * \note the bit value "0" indicates an empty list. */
    UINT64 bucket_bitmap[ BITMAP_ARRAY_SIZE ]; // 0 == empty
    /** find the next nonzero bit in the bitmap.
     * \note races through the bitmap a 64-bit doubleword at a time. */
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
    /** set a bit in the bitmap corresponding to a bucket */
    void set_bit(int bucket) {
        bucket_bitmap[bucket >> 6] |=  (1ULL << (bucket & 0x3F));
    }
    /** clear a bit in the bitmap corresponding to a bucket */
    void clear_bit(int bucket) {
        bucket_bitmap[bucket >> 6] &= ~(1ULL << (bucket & 0x3F));
    }
    /** put an Extent into the correct bucket.
     * \note assumes Extent::size has been properly populated.
     * \note calls set_bit to mark the bucket as nonempty. */
    void add_to_bucket( Extent * e ) {
        int b = size_to_bucket(e->size);
        ExtentBucketList * bucket = &buckets[b];
        bucket->add(e);
        if (bucket->get_cnt() == 1)
            set_bit(b);
    }
    /** remove an Extent from a bucket list.
     * \note assumes Extent::size is populated.
     * \note calls clear_bit if a bucket list has been made empty. */
    void remove_from_bucket( Extent * e ) {
        int b = size_to_bucket(e->size);
        ExtentBucketList * bucket = &buckets[b];
        bucket->remove(e);
        if (bucket->get_cnt() == 0)
            clear_bit(b);
    }
    // @}  end of bucket list
public:
    /** @name constructor/destructor */
    // @{
    /** constructor starts with an empty list.
     *
     * User must populate the list after creating.  When this
     * object is created, there are no Extent objects.
     * Assumption is the user will add all of the initial entries
     * immediately after creation. */
    Extents(void);
    /** destructor destroys all Extent objects. */
    ~Extents(void);
    /** for initial setup; add a free Extent.
     * \note This function assumes all entries added are in order,
     *  that is, each Extent offset plus size matches the following
     *  Extent offset.  Undefined behavior will result if this is not
     *  the case.
     * \note This function must not be called after normal operations
     *  have begun! */
    void add( off_t _offset, UINT32 _size );
    /** for initial setup; add a used Extent.
     * \note This function assumes all entries added are in order,
     *  that is, each Extent offset plus size matches the following
     *  Extent offset.  Undefined behavior will result if this is not
     *  the case.
     * \note This function must not be called after normal operations
     *  have begun! */
    void add( off_t _offset, UINT32 _size, UINT32 _id );
    // @}
    /** @name Debugging */
    // @{
    /** print out on TTY a list of all Extent objects */
    void print(void);
    // @}
    /** @name Access methods */
    // @{
    /** locate an Extent by its ID number.
     * \param id the unique 32-bit identifier of the Extent.
     * \return a pointer to the Extent.
     * \note The returned Extent object is still on an internal linked
     *  list.  The user does not have to do anything to release the Extent
     *  back to this object when the user is done with it.
     * \note The user must not reference this Extent pointer after
     *  performing any other manipulations on the Extents object, because
     *  this Extent object may have been altered or deleted. */
    Extent * find_id( UINT32 id );
    /** allocate a new Extent of a certain size.
     * \param size The size of the Extent required.
     * \return a pointer to the Extent.
     * \note The returned Extent object is still on an internal linked
     *  list.  The user does not have to do anything to release the Extent
     *  back to this object when the user is done with it.
     * \note The user must not reference this Extent pointer after
     *  performing any other manipulations on the Extents object, because
     *  this Extent object may have been altered or deleted.
     *
     * This method allocates a new Extent from the free bucket list. 
     * It attempts to find a best-matched Extent.  If it can't find an
     * exact match, it looks higher in the list until a free region is
     * found that can be fragmented. */
    Extent * alloc( UINT32 size ) {
        return _alloc( 0, size );
    }
    /** change the size of an allocation. 
     * \param id The identifier of the old Extent to modify.
     * \param new_size the new size desired for the Extent. 
     * \return a pointer to the Extent.
     * \note This does not resize the existing extent!  It will in most cases
     *  free the old Extent and create a new one in a completely different
     *  place.
     *
     * This method frees an old Extent and allocates a new Extent of a
     * different size, but re-uses the same identifier.  This method is 
     * useful for applications which store Extent IDs within other data
     * blocks, and would prefer not to update the parent object if the
     * child object has to be resized. */
    Extent * realloc( UINT32 id, UINT32 new_size ) {
        return _alloc(id,new_size);
    }
    /** Free up an Extent that is no longer needed.
     * \param id The unique 32-bit identifier of the Extent.
     * 
     * This method returns the Extent back to the free bucket queue.
     * If the region described is adjacent to other free regions, the
     * two regions are coalesced into a single larger free Extent. */
    void free_id( UINT32 id );
    /** Free up an Extent that is no longer needed.
     * \param e the Extent to be freed.
     * This method returns the Extent back to the free bucket queue.
     * If the region described is adjacent to other free regions, the
     * two regions are coalesced into a single larger free Extent. */
    void free( Extent * e );
    // @}
    /** @name Traversal */
    // @{
    /** get the first Extent on the ordered list.
     * \return the first Extent on the ordered Extent list, or NULL if the 
     *  list is empty (this is technically not possible).
     * \note This is used in conjunction with the get_next() method
     *  to walk the linked list of all Extent objects in order. This is
     *  useful for debugging and for saving the Extents contents offline.
     * \note The Extents object must \b not be manipulated while walking
     *  the linked list; to do so may result in incorrect operation. */
    Extent * get_head(void) { return list.get_head(); }
    /** get the next Extent on the ordered list
     * \param e the previous Extent on the ordered list
     * \return the next Extent on the list
     * \note The Extents object must \b not be manipulated while walking
     *  the linked list; to do so may result in incorrect operation. */
    Extent * get_next(Extent * e) { return list.get_next(e); }
    /** return the count of used extents.
     * \return the count of used extents. */
    int get_count_used(void) { return count_used; }
    /** return the count of free extents.
     * \return the count of free extents. */
    int get_count_free(void) { return count_free; }
    // @}
};

#endif /* __EXTENT_MAP_H__ */
