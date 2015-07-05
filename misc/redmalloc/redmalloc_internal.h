
/*
    This file is part of the "pfkutils" tools written by Phil Knaack
    (pfk@pfk.org).
    Copyright (C) 2008  Phillip F Knaack

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

// someone outside redmalloc must provide these functions.
extern "C" void * _real_malloc( int size );
extern "C" void * _real_realloc( void * ptr, int size );
extern "C" void   _real_free( void * ptr );

#if 1   // enable this for vxworks.
#define   LOCK()      intLock()
#define   UNLOCK(v)   intUnlock(v)
#define   DLL2BAIL(reason) { pprintf reason; taskSuspend( 0 ); }
#else
#define   LOCK()      0
#define   UNLOCK(v)   
#endif

#include "redmalloc_dll2.H"

// phil knaack's in-use-block.

struct pkiub {
    static const int REDZONE_SIZE  = 128;   // minimum 16
    static const int ZONE_VALUE    = 0x5a;
    static const int DEAD_VALUE    = 0x54;
    static const int LIVE_VALUE    = 0x5c;
    static const int RANDOM_XOR    = 0x35a1876c;

    static const int NO_ERROR        =  0;
    static const int ZONE1_WRONG     =  1;
    static const int ZONE2_WRONG     =  2;
    static const int ZONE3_WRONG     =  4;
    static const int ZONE4_WRONG     =  8;
    static const int INVALID_PTR_MG1 = 16;
    static const int INVALID_PTR_MG2 = 32;
    static const int INVALID_POINTER = 64;

    static const int TASK_NAME_WIDTH = 12;

    static pkiub  * find_pointer( void * ptr );
    void          * get_memory( int size, int pc );
    int             check_zones( bool ignore_history );
    void            free_memory( void );

    LListLinks <pkiub> links[1];
    char *   pointer;
    int      size;
    int      pc;
    int      bad;
    char     task_name[TASK_NAME_WIDTH];

private:
    static void * operator new    ( size_t sz ) { } // can't create with new
    static void   operator delete ( void * p  ) { } // can't delete
};
typedef LList <pkiub,0>   pkiub_list;

struct pkiub_array {
    static const int ALLOC_AT_ONCE = 100;
    LListLinks <pkiub_array> links[1];
    pkiub blocks[ALLOC_AT_ONCE];

    pkiub_array( void );
    static void * operator new    ( size_t sz ) { return _real_malloc( sz ); }
    static void   operator delete ( void * p  ) {        _real_free( p );    }
};
typedef LList<pkiub_array,0> pkiub_array_list;


struct display_item {
    LListLinks <display_item>  links[1];
    char *  pointer;
    int     size;
    int     pc;
    int     err;
    char    task_name2[ pkiub::TASK_NAME_WIDTH + 1 ];

    display_item( pkiub * iub, int _err ) {
        pointer = iub->pointer;
        size = iub->size;
        pc = iub->pc;
        memcpy( task_name2, iub->task_name, pkiub::TASK_NAME_WIDTH );
        task_name2[ pkiub::TASK_NAME_WIDTH ] = 0;
        err = _err;
    }
    int display( bool is_error ) {
        int (*func)(const char *,...);
        func = is_error ? pprintf : printf;
        func( "%sblock %#x size %d pc %#x "
              "task %s err %#x\n",
              is_error ? "*** REDMALLOC ERROR:" : "",
              pointer, size, pc, 
              task_name2,
              err );
        return size;
    }

    static void * operator new    ( size_t sz ) { return _real_malloc( sz ); }
    static void   operator delete ( void * p  ) {        _real_free( p );    }
};
typedef LList <display_item,0>  display_item_list;
