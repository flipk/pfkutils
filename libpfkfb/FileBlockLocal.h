/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/

/** \file FileBlockLocal.h
 * \brief All private data types to FileBlockLocal implementation.
 */

#ifndef __FILE_BLOCK_LOCAL_H__
#define __FILE_BLOCK_LOCAL_H__

#include "FileBlock_iface.h"
#include "dll3.h"

#ifdef __GNUC__
# if __GNUC__ >= 6
#  define ALLOW_THROWS noexcept(false)
# else
#  define ALLOW_THROWS
# endif
#else
# define ALLOW_THROWS
#endif

/** a data type for AU id numbers.  */
typedef uint32_t   FB_AUN_T;
/** a data type for AU id numbers when encoded in a file */
typedef UINT32_t FB_AUN_t;

/** 
 * fundamental size of an allocation unit.  every allocation is
 * a multiple of this. note that FB_AUN_T is 31 bits long, therefore
 * maximum file size is 512 * 0x7ffffff = ~1TB.
 */
#define AU_SIZE 512

/**
 * calculate the number of AUs required to hold a size.  performs
 * the proper round-up in case of not even number of AUs.
 */
#define SIZE_TO_AUS(size) (((size)+(AU_SIZE-1)) / AU_SIZE)

/** a template which adds a method to the usual BlockCacheT, which
    supports seeking in the file by AUN instead of byte offset. */
template <class T>
struct BlockCacheAUT : public BlockCacheT <T> {
    BlockCacheAUT(BlockCache * bc) : BlockCacheT<T>(bc) { my_aun = 0; }
    /** override parent type's get method with a method that takes
     * an AUN instead of a file offset. 
     * \param aun        the AUN in the file corresponding to this block.
     * \param for_write  if true, do not read from the file, instead prepare
     *        an empty buffer for the application to fill in.
     * \return  true if successful, false if some failure ocurred.
     */
    bool get(FB_AUN_T aun, bool for_write = false) {
        my_aun = aun;
        off_t pos = ((off_t)aun * AU_SIZE);
        return BlockCacheT<T>::get(pos,for_write);
    }
    /** the AUN is a private field, this is an accessor. */
    FB_AUN_T get_aun(void) {
        return my_aun;
    }
private:
    FB_AUN_T my_aun;
};

/** 
 * overall management information about a file.
 * contains information such as statistics on space usage
 * and first AUN, AUIDs, and AUID stack.
 */
struct InfoBlock {
    /** the file must have a valid signature to be used. */
    static const uint32_t SIGNATURE = 0x04f78c2a;
    UINT32_t  signature;      /**< value is SIGNATURE */
    UINT32_t  num_buckets;    /**< must match BucketList::NUM_BUCKETS */
    UINT16_t  au_size;        /**< must match AU_SIZE */
    UINT32_t  used_aus;       /**< count of used AUs */
    UINT32_t  free_aus;       /**< count of free AUs */
    UINT32_t  used_extents;   /**< count of used regions */
    UINT32_t  free_extents;   /**< count of free regions */
    FB_AUN_t  first_au;       /**< number of first AU */
    UINT32_t  num_aus;        /**< count of how many AUs in the file */
    FB_AUID_t auid_top;       /**< largest auid allocated so far+1 */
    UINT32_t  auid_stack_top; /**< top of the auid free stack */
};

/**
 * mapping information on data info blocks.
 * each entry is either zero or points to a DataInfoBlock structure.
 */
struct DataInfoPtrs {
    static const int MAX_DATA_INFOS = 64;
    /** either zero or points to a DataInfoBlock structure */
    FB_AUID_t ptrs[MAX_DATA_INFOS];
};

/**
 * structure for linked list heads of free blocks.
 * each entry corresponds to different size blocks.  For example,
 * index [0] is for 1-AU blocks.  index [1] is for 2-AU blocks, etc.
 */
struct BucketList {
    static const int NUM_BUCKETS = 4096;
    /** each entry is either zero or the AUN of a linked list head.
     * Also, if a list is nonempty, there will be a bit set in the
     * bitmap, too. \see BucketBitmap
     */
    FB_AUN_t list_head[NUM_BUCKETS];
};

#define FILE_BLOCK_MAXIMUM_ALLOCATION_SIZE \
    ((BucketList::NUM_BUCKETS - 2) * AU_SIZE)

/**
 * bitmap for the bucket lists.  If a bit is set, it means the
 * corresponding bucket list is nonempty.
 */
struct BucketBitmap {
    // the 32's in this class are not AU_SIZE, they
    // are the number of bits in a uint32.
    /** the bits themselves live here. */
    UINT32_t longs[BucketList::NUM_BUCKETS/32];
    /**
     * change value of a bit in the bitmap.
     * \param bucket  which bit/corresponding bucket to alter.
     * \param value   whether to set or clear that bit.
     */
    void set_bit(int bucket, bool value) {
        int whichbit  = bucket % 32;
        int whichlong = bucket / 32;
        if (value)
            longs[whichlong].set(
                longs[whichlong].get() |  (1 << whichbit)
                );
        else
            longs[whichlong].set(
                longs[whichlong].get() & ~(1 << whichbit)
                );
    }
    /**
     * retrieve the value of a bit.
     * \param bucket  which bit/corresponding bucket to check
     * \return  true if the bit is set.
     */
    bool get_bit(int bucket) {
        int whichbit  = bucket % 32;
        int whichlong = bucket / 32;
        if (longs[whichlong].get() & (1 << whichbit))
            return true;
        return false;
    }
};

/**
 * an AUID L1 table.  there are two of these included in the FileHeader,
 * one used for AUID-to-AUN translation, and the other used for the
 * free-AUID stack. the top 12 bits of a 32-bit value are used to index 
 * this table.  the AUID translation table is indexed by the AUID value,
 * while the free-AUID stack is indexed by the InfoBlock::auid_stack_top
 * value.
 */
struct AuidL1Tab {
    static const int L1_ENTRIES = 4096;
    /** the top 12 bits of a 32-bit value are used to index 
     * this table. */
    static int auid_to_l1_index(uint32_t v) { return (v >> 20) & 0xFFF; }
    FB_AUN_t  entries[L1_ENTRIES];
};

/**
 * an AUID L2 or L3 table.  these are linked from the two AuidL1Tab tables,
 * used for AUID-to-AUN translation and free-AUID stack.  they are
 * dynamically allocated as needed.  the remaining 20 bits of a 32-bit
 * value are used to index these tables; bottom 10 for the L3 and the next
 * 10 bits for the L2 table.
 */
struct AuidL23Tab {
    static const int L23_ENTRIES = 1024;
    static int auid_to_l2_index(uint32_t v) { return (v >> 10) & 0x3FF; }
    static int auid_to_l3_index(uint32_t v) { return (v >>  0) & 0x3FF; }
    FB_AUN_t  entries[L23_ENTRIES];
};

/**
 * this structure occurs at the beginning of the file and is referenced
 * constantly. 
 */
struct _FileHeader {
    InfoBlock      info;             /**< all the stats you ever wanted */
    DataInfoPtrs   data_info_ptrs;   /**< name to AUID mapping */
    BucketList     bucket_list;      /**< arrays of bucket head ptrs */
    BucketBitmap   bucket_bitmap;    /**< bitmaps of nonempty buckets */
    AuidL1Tab      auid_l1;          /**< AUID-to-AUN mapping tables */
    AuidL1Tab      auid_stack_l1;    /**< AUID free stack tables */
};
/**
 * a perversion of BlockCacheT which knows only FileHeader and where it
 * goes.
 */
class FileHeader : public BlockCacheAUT <_FileHeader> {
public:
    static const FB_AUN_T FILE_HEADER_AUN = 0;
    FileHeader(BlockCache *bc) : BlockCacheAUT <_FileHeader>(bc) { }
    bool get(bool for_write=false) {
        return BlockCacheAUT <_FileHeader>::get(FILE_HEADER_AUN,for_write);
    }
};

/**
 * this structure is at the start of every region.
 * it encodes a linked list to the previous region and the size
 * of this region (so that an AUN to the next region can be recovered).
 * this is how we know when to coalesce free regions, by looking at the
 * next and previous pointers and checking their used-bits.
 * used regions also store the AUID (optionally).
 * free regions also store the linked list pointers for the bucket list.
 */
class _AUHead {
    /** if this bit is set in size_and_used, the region is in use. */
    static const uint32_t USED_MASK = 0x80000000;
    /** these 31 bits are used for the size of the region. */
    static const uint32_t SIZE_MASK = 0x7fffffff;
public:
    static const int used_size = 12;
    /** pointer to the previous region in the file. */
    FB_AUN_t  prev;
private:
    /** this field encodes both the size (31 bits) and the
     * used/free flag (1 bit).  \see USED_MASK, \see SIZE_MASK. */
    UINT32_t  size_and_used;
    /** a used region puts the AUID here, while a free region puts
     * the bucket_prev pointer. */
    FB_AUN_t  one;
    /** a free region puts the bucket_next here.  a used region 
     * does not use this space.  it is available for use by the
     * application. */
    FB_AUN_t  two;
public:
    /** extract the number of AUNs this region consumes from
     * the size_and_used member.
     * \return size in AUs
     */
    int size(void) { 
        return size_and_used.get() & SIZE_MASK;
    }
    /**
     * set the size of this region.
     * \param size   the size in AUs of the region.
     */
    void size(int size) {
        size_and_used.set(
            (size_and_used.get() & USED_MASK) |
            (size & SIZE_MASK));
    }
    /** query the used-bit for this region.
     * \return  true if the region is used, false if free.
     */
    bool used(void) {
        return (size_and_used.get() & USED_MASK) != 0;
    }
    /**
     * set the used-bit in this region.
     * \param used   set to true for a used region, false if free.
     */
    void used(bool used) {
        size_and_used.set(
            (size_and_used.get() & SIZE_MASK) |
            (used ? USED_MASK : 0));
    }
    /** fetch the AUID if this is a used-region. */
    FB_AUID_T auid        ( void        ) { return one.get(); }
    /** set the AUID if this is a used-region. */
    void      auid        ( FB_AUID_T v ) { one.set(v); }
    /** fetch the bucket_next for a free region */
    FB_AUN_T bucket_next ( void       ) { return one.get(); }
    /** set the bucket_next for a free region */
    void     bucket_next ( FB_AUN_T v ) { one.set(v); }
    /** fetch the bucket_prev for a free region */
    FB_AUN_T bucket_prev ( void       ) { return two.get(); }
    /** set the bucket_prev for a free region */
    void     bucket_prev ( FB_AUN_T v ) { two.set(v); }
};
/** a BlockCache accessor template for the _AUHead type. */
typedef BlockCacheAUT <_AUHead> AUHead;

/** map a text name to an AUID.  referenced by DataInfoPtrs::ptrs. */
struct _DataInfoBlock {
    static const int MAX_INFO_NAME = 128;
    char      info_name[MAX_INFO_NAME];
    FB_AUID_t info_auid;
};
typedef FileBlockT <_DataInfoBlock> DataInfoBlock;

//
class FileBlockInt;
typedef DLL3::List <FileBlockInt, 1, false>  FileBlockList_t;

/**
 * internal representation of a FileBlock.  Just adds some linked
 * list info so we can keep track of it.
 */
class FileBlockInt : public FileBlock,
                     public FileBlockList_t::Links
{
public:
    virtual ~FileBlockInt(void) ALLOW_THROWS { }
    void set_auid(uint32_t _auid) { auid = _auid; }
    void set_bcb(BlockCacheBlock * _bcb) { bcb = _bcb; }
    BlockCacheBlock *get_bcb(void) { return bcb; }
};

//
class L2L3s;

/**
 * the private implementation of FileBlockInterface.
 * the real meat happens here. 
 */
class FileBlockLocal : public FileBlockInterface {
    BlockCache * bc;
    /** a list of all FileBlockInt objects which have been
     * given to the general public.  */
    FileBlockList_t  active_blocks;
    /** a perpetual pointer to the header of the file. */
    FileHeader  fh;
    /** @name bucket management */
    // @{
    /** find first used bucket with usable region size.
     * \param num_aus  the number of AUs required.
     * \return  index into bucket array of a nonempty bucket list.
     */
    int       ffu_bucket     ( int num_aus );
    /** remove first region from a specified bucket.
     * \param bucket   the number of the bucket list to access.
     * \param au       pointer to a user's AUHead to populate with
     *           info from the AU
     * \return  true if the bucket was dequeued, false if
     *            bucket list was empty.
     */
    bool      dequeue_bucketn( int bucket,  AUHead * au );
    /** remove region from first matching bucket.
     * \param num_aus   the size region required, in AUs
     * \param au    pointer to a user's AUHead to populate with
     *           info from the AU
     */
    void      dequeue_bucket ( int num_aus, AUHead * au ) {
        dequeue_bucketn(ffu_bucket(num_aus), au);
    }
    /** return a region to the appropriate bucket.
     * \param  au    the AU to return to a bucket.
     */
    void      enqueue_bucket ( AUHead * au );
    /** remove a region from whatever bucket it is a member of.
     * \param au    the AU to remove from its bucket.
     */
    void      remove_bucket  ( AUHead * au );
    /** calculate the bucket index for a required size.
     * \param num_aus  the size of a region in AUs
     * \return  the bucket index this region size should be in. 
     */
    static int aus_to_bucket (int num_aus) {
        if (num_aus == 0)   return BucketList::NUM_BUCKETS-1;
        if ((num_aus-1) >= (BucketList::NUM_BUCKETS-1))
            return BucketList::NUM_BUCKETS-2;
        return (num_aus-1);
    }
    // @}
    /** @name AUN management */
    // @{
    /** allocate a region of the file.
     * \param desired_aun   desired position, must be a free bucket.
     *        if zero, buckets are used to allocate dynamically.
     * \param au    the user's AUHead object to fill in.
     * \param size  the number of AUs required.
     * \return   the AUN of the allocated space.
     */
    FB_AUN_T  alloc_aun      ( FB_AUN_T desired_aun, AUHead * au, int size );
    /** allocate a region of the file.
     * \param au    the user's AUHead object to fill in.
     * \param size  the number of AUs required.
     * \return   the AUN of the allocated space.
     */
    FB_AUN_T  alloc_aun      ( AUHead * au, int size ) {
        return alloc_aun( 0, au, size );
    }
    /** allocate a region of the file, where AUHead is not required.
     * \param size  the number of AUs required.
     * \return  the AUN of the allocated space.
     */
    FB_AUN_T  alloc_aun      ( int size ) {
        AUHead au(bc); return alloc_aun(&au,size);
    }
    /** free a region of the file.
     * \param aun    the AUN of the region to free.
     */
    void      free_aun       ( FB_AUN_T aun );
    /** fetch a region of the file, skipping the AUHead.
     * this method looks up how large the region is, and returns an
     * object appropriately sized for that region.
     * \param aun   the AUN where the data begins.
     * \param for_write   if true, the data will not be read from the
     *             file, instead some memory will be allocated and set 
     *             to zeroes, so the application can start writing.
     * \return a FileBlock object referencing that part of the file.
     * \note Be sure to call FileBlock::mark_dirty method if you have
     *    modified any data in this region!
     */
    FileBlockInt *  get_aun  ( FB_AUN_T aun, bool for_write=false );
    // @}
    /** @name AUID management */
    // @{
    /** allocate a new AUID and map it to an AUN.
     * \param aun   the AUN to be mapped.
     * \return  the new AUID.
     */
    FB_AUID_T alloc_auid     ( FB_AUN_T aun );
    /** map an AUID to a different AUN.
     * \param auid   the AUID to be altered.
     * \param aun    the AUN to be mapped to this AUID.
     */
    void      rename_auid    ( FB_AUID_T auid, FB_AUN_T aun );
    /** lookup an AUID and return its corresponding AUN.
     * \param auid    the AUID to look up.
     * \return   the AUN mapped to this AUID.
     */
    FB_AUN_T  translate_auid ( FB_AUID_T auid );
    /** free up an AUID.
     * \param auid    the AUID to free.
     */
    void      free_auid      ( FB_AUID_T auid );
    /** modify an entry in the free-AUID stack.
     * \param  index    the entry in the stack to modify.
     * \param  auid     the value to write to the specified entry.
     */
    void      write_stack    ( uint32_t index, FB_AUID_T auid );
    /** read out of the free-AUID stack.
     * \param index    the entry to read.
     * \return   the AUID stored at that position in the stack.
     */
    FB_AUID_T lookup_stack   ( uint32_t index );
    // @}
    /** @name Compaction */
    // @{
    /** walk an L2 table collecting L3 AUNs, used by compact.
     * \param l   pointer to L2L3AUNList
     * \param aun  the AUN of the L2 table to walk
     * \param ty   the TableType of the L2 table to walk.
     *
     * See also: \ref addent
     */
    void      walkl2         ( void * l, FB_AUN_T aun, int ty );
    /** move a block to a different location in the file.
     * doesn't matter if this is an L2/L3 table or a user AUID.
     * reallocs a used block, or if its an L2/L3 table, uses the
     * AUN interface.
     * \param l     L2L3s list
     * \param auid  the AUID of the block to move; if its an L2/L3 table
     *              this should be zero.
     * \param aun   the AUN of the block to move.
     * \param to_aun desired AUN position to move block to; if zero,
     *               will be dynamically allocated.
     */
    void      move_unit   ( L2L3s *l, FB_AUID_T auid,
                            FB_AUN_T aun, FB_AUN_T to_aun );
    FB_AUID_T realloc     ( FB_AUID_T auid, FB_AUN_T to_aun, int new_size );
    // @}
public:
    /*       */             FileBlockLocal     ( BlockCache * _bc );
    /*virtual*/            ~FileBlockLocal     ( void );
    static      bool        valid_file         ( BlockCache * bc );
    static      void        init_file          ( BlockCache * bc );
    /*virtual*/ FB_AUID_T   alloc              ( int size );
    /*virtual*/ void        free               ( FB_AUID_T auid );
    /*virtual*/ FB_AUID_T   realloc            ( FB_AUID_T auid, int new_size );
    /*virtual*/ FB_AUID_T   get_data_info_block( char * info_name );
    /*virtual*/ void        set_data_info_block( FB_AUID_T auid,
                                                 char *info_name );
    /*virtual*/ void        del_data_info_block( char * info_name );
    /*virtual*/ FileBlock * get                ( FB_AUID_T auid,
                                                 bool for_write = false );
    /*virtual*/ void        release            ( FileBlock * blk,
                                                 bool dirty = false );
    /*virtual*/ void        flush              ( void );
    /*virtual*/ void        compact            ( FileBlockCompactionStatusFunc
                                                 func, void * arg );
    /*virtual*/ void        get_stats          ( FileBlockStats * stats );

    /** debug information.
     * \param verbose   set to true if you want it to display everything
     *       it knows about the metadata.  set to false to show only errors.
     */
    void      validate       ( bool verbose=true );
};

#endif /* __FILE_BLOCK_LOCAL_H__ */
