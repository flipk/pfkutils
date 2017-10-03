/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "dll2.H"

// each entry describes a region of memory.
// the entry describes if this region is used or free.
// each entry resides on a list, in order by address.
// each entry, if currently FREE, is on a list corresponding to bucket size.
// each entry, if currently USED, is on a hash by start address.
// there is a pool of free entries which come from entry containers which
//    come from e_exec_alloc pool.

// malloc:
//   round size up to nearest 16
//   start at smallest bucket size with available entries.
//   when entry found, if size is exact match:
//      change status to USED
//      remove from bucket list
//      add to address hash
//   if size is not exact match:
//      split into two entries:
//         new entry:
//            extent of size required
//            marked as USED
//            add to address hash
//            insert into map list prior to original entry
//         original entry:
//            extent as remainder of original size
//            marked as FREE
//            if now belongs in different bucket, remove+add

// free:
//   find address on hash
//   remove from hash
//   change status to FREE
//   if previous entry is also FREE, coalesce with it
//   if next entry is also FREE, coalesce with it

// coalesce:
//   update one entry to contain both extents
//   remove the other entry

enum memory_region_lists {
    REGION_LIST_SORTED,
    REGION_BUCKET_LIST_POINTER_HASH,
    REGION_NUM_LISTS
};

class memory_region {
    static const int USED_BIT  = 0x80000000;
    static const int SIZE_MASK = 0x7fffffff;
    ULONG start;
    ULONG size;

public:
    LListLinks<memory_region>  links[ REGION_NUM_LISTS ];
    int hash_key(void) {
        // this method used by LListHash class
        return (int) (start >> 4 );
    }
    static int hash_key( ULONG start ) {
        // used by free() to search the LListHash
        return (int) (start >> 4 );
    }
    memory_region( void ) {
        start = 0; size = 0;
    }
    void init( ULONG _start, ULONG _size ) {
        start = _start; size = _size;
    }
    ULONG start_get( void ) {
        return start;
    }
    void start_set( ULONG v ) {
        size_set( end_get() - v + 1 );
        start = v;
    }
    ULONG end_get( void ) {
        return start+(size&SIZE_MASK)-1;
    }
    void end_set( ULONG v ) {
        size_set( v-start+1 );
    }
    ULONG size_get( void ) {
        return size & SIZE_MASK;
    }
    void size_set( ULONG v ) {
        size = (size & USED_BIT) + v;
    }
    bool is_used( void ) {
        if (( size & USED_BIT ) != 0 )
            return true;
        return false;
    }
    void set_used( void ) {
        size |=  USED_BIT;
    }
    void set_free( void ) {
        size &= ~USED_BIT;
    }
};

typedef LList     <memory_region,
                   REGION_LIST_SORTED>               Region_list_sorted;
typedef LList     <memory_region,
                   REGION_LIST_SORTED>               Region_free_list;
typedef LList     <memory_region,
                   REGION_BUCKET_LIST_POINTER_HASH>  Region_bucket_list;
typedef LListHash <memory_region,
                   REGION_BUCKET_LIST_POINTER_HASH>  Region_pointer_hash;

class memory_region_page {
    static const int NUM_REGIONS = 127;
    memory_region regions[ NUM_REGIONS ];

public:
    LListLinks <memory_region_page> links[1];
    memory_region_page( Region_free_list * l ) {
        for ( int i = 0; i < NUM_REGIONS; i++ )
            l->add( &regions[i] );
    }
    static void * operator new( size_t s ) {
        // xxx change for exec
        return malloc( s );
    }
};

typedef LList <memory_region_page,0> Region_page_list;

class memory_region_manager {
    static const int BUCKET_MIN  =   16;
    static const int BUCKET_MAX  = 2048;
    static const int NUM_BUCKETS =   32; // 16 thru 2048

// management of free region-control structures
    Region_page_list      region_page_list;
    Region_free_list      region_free_list;

// management of regions themselves
    Region_list_sorted    regions;
    Region_bucket_list    free_buckets   [ NUM_BUCKETS ];
    Region_pointer_hash   used_pointer_hash;

// stats
    ULONG pool_size;
    ULONG bytes_used;
    ULONG peak_bytes_used;
    ULONG used_regions;

    ULONG memory_base;

    memory_region * alloc_memory_region( void ) {
        if ( region_free_list.get_cnt() == 0 )
            region_page_list.add(
                new memory_region_page( &region_free_list ));
        return region_free_list.dequeue_head();
    }
    void mark_region_used( memory_region * m ) {
        m->set_used();
        used_regions++;
        bytes_used += m->size_get();
        if ( bytes_used > peak_bytes_used )
            peak_bytes_used = bytes_used;
    }
    void mark_region_free( memory_region * m ) {
        m->set_free();
        used_regions--;
        bytes_used -= m->size_get();
    }
    static ULONG size_to_bucket( ULONG sz ) {
        ULONG ret = sz / (BUCKET_MAX/NUM_BUCKETS);
        if ( ret >= NUM_BUCKETS ) ret = NUM_BUCKETS-1;
        return ret;
    }

public:
    memory_region_manager( ULONG size, ULONG hashsize );
    ~memory_region_manager( void );

    // user interface
    void * malloc( ULONG size );
    void free( void * pointer );

    // stats
    ULONG get_used( void ) {
        return bytes_used;
    }
    ULONG get_peak_used( void ) {
        return peak_bytes_used;
    }
    ULONG get_free( void ) {
        return pool_size - bytes_used;
    }
    ULONG get_regions( void ) {
        return regions.get_cnt();
    }
    ULONG get_used_regions( void ) {
        return used_regions;
    }
    ULONG get_free_regions( void ) {
        return regions.get_cnt() - used_regions;
    }
};
