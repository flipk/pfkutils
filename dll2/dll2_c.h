
/*
    This file is part of the "pkutils" tools written by Phil Knaack
    (pknaack1@netscape.net).
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

/* define following to "0" to save some CPU time on the list operations--
   beware of the safety checks it turns off, though. 
   you should always have them turned on during testing. */

#ifndef DLL2_ONLIST_VALIDATIONS
#define DLL2_ONLIST_VALIDATIONS 1
#endif

#ifndef DLL2_MAGIC_VALIDATIONS
#define DLL2_MAGIC_VALIDATIONS 1
#endif

/* Must include "DLL2_LINKS links[ .. ]" in every
   data type that is put in a list. */

typedef struct {
    void             * links_next;
    void             * links_prev;
    struct DLL2_LIST * links_onlist;
#if DLL2_MAGIC_VALIDATIONS
    int                links_magic;
#endif
} DLL2_LINKS;

typedef struct DLL2_LIST {
    void * list_head;
    void * list_tail;
    int list_count;
    int list_index;
#if DLL2_MAGIC_VALIDATIONS
    int list_magic;
#endif
} DLL2_LIST;

/* init/deinit macros used by public */

#define DLL2_LIST_INIT(list,listindex) \
    do { \
        DLL2_LIST_VALIDATE_NOTINIT(list); \
        (list)->list_head = (list)->list_tail = 0; \
        (list)->list_count = 0; \
        (list)->list_index = listindex; \
        DLL2_INIT_LIST_MAGIC(list); \
    } while ( 0 )
#define DLL2_ITEM_INIT(item) \
    do { \
        int __dll2_internal_cnt; \
        for ( __dll2_internal_cnt = 0; \
              __dll2_internal_cnt < DLL2_DIM((item)->links); \
              __dll2_internal_cnt++ ) \
        { \
            DLL2_LINKS_VALIDATE_NOTINIT(item,__dll2_internal_cnt); \
            (item)->links[__dll2_internal_cnt].links_next = 0; \
            (item)->links[__dll2_internal_cnt].links_prev = 0; \
            (item)->links[__dll2_internal_cnt].links_onlist = 0; \
            DLL2_INIT_LINKS_MAGIC(item,__dll2_internal_cnt); \
        } \
    } while ( 0 )
#define DLL2_LIST_DEINIT(list) \
    do { \
        if ( (list)->list_count != 0 || \
             (list)->list_head  != 0 || \
             (list)->list_tail  != 0 ) \
        { \
            (void) dll2_error( __FILE__, __LINE__, \
                               "dll2_list_deinit not empty!" ); \
        } \
        DLL2_DEINIT_LIST_MAGIC(list); \
    } while ( 0 )
#define DLL2_ITEM_DEINIT(item) \
    do { \
        int __dll2_internal_cnt; \
        for ( __dll2_internal_cnt = 0; \
              __dll2_internal_cnt < DLL2_DIM((item)->links); \
              __dll2_internal_cnt++ ) \
        { \
            if ( DLL2_NCMP_ONLIST_F(item,__dll2_internal_cnt,0) || \
                 (item)->links[__dll2_internal_cnt].links_next   != 0 || \
                 (item)->links[__dll2_internal_cnt].links_prev   != 0 ) \
            { \
                (void) dll2_error( __FILE__, __LINE__, \
                                   "dll2_item_deinit still on a list!" ); \
            } \
            DLL2_DEINIT_LINKS_MAGIC(item,__dll2_internal_cnt); \
        } \
    } while ( 0 )

/* onlist methods used by public */

#define DLL2_LIST_ONLIST(item,listindex) \
    ((item)->links[listindex].links_onlist != 0)
#define DLL2_LIST_ONTHISLIST(list,item) \
    ((item)->links[(list)->list_index].links_onlist == (list))

/* list update methods used by public */

#define DLL2_LIST_ADD(list,item) \
    dll2_list_add((list),DLL2_LINKS_OFFSET(item),(item),__FILE__,__LINE__)
#define DLL2_LIST_REMOVE(list,item) \
    dll2_list_remove((list),DLL2_LINKS_OFFSET(item),(item),__FILE__,__LINE__)
#define DLL2_LIST_ADD_AFTER(list,existing,item)   \
    dll2_list_add_after((list),DLL2_LINKS_OFFSET(item), \
                        (existing),(item),__FILE__,__LINE__)
#define DLL2_LIST_ADD_BEFORE(list,existing,item)  \
    dll2_list_add_before((list),DLL2_LINKS_OFFSET(item), \
                         (existing),(item),__FILE__,__LINE__)

/* list-walking methods used by public */

#define DLL2_LIST_SIZE(list)              ((list)->list_count)
#define DLL2_LIST_HEAD(list)              ((list)->list_head)
#define DLL2_LIST_TAIL(list)              ((list)->list_tail)

#if 0   /* HASH IMPLEMENTATION INCOMPLETE */
typedef struct {
    int         hash_hashsize;
    DLL2_LIST * hash_hash;
    int         hash_count;
#if DLL2_MAGIC_VALIDATIONS
    int         hash_magic;
#endif
} DLL2_LIST_HASH;

#define DLL2_HASHV(list,key) (key % (list)->hashsize)
#define DLL2_LISTHASH_INIT(list,hash_size,malloc_func,listindex) \
    do { \
        int i; \
        (list)->hashsize = (hash_size); \
        (list)->hash = (DLL2_LIST*) \
            malloc_func( sizeof(DLL2_LIST) * (list)->hashsize ); \
        (list)->list_count = 0; \
        for ( i = 0; i < (list)->hashsize; i++ ) \
            DLL2_LIST_INIT( &((list)->hash), listindex ); \
    } while ( 0 )  xxx magic
#endif    /* if 0 HASH IMPLEMENTATION INCOMPLETE */

/* everything below this line is INTERNAL to the DLL2 API,
   and shouldn't need accessing by the public. */

#define DLL2_LINKS_MAGIC 0x4bf4e59c
#define DLL2_LIST_MAGIC  0x5c2df716
#define DLL2_HASH_MAGIC  0x427ec79c

#if DLL2_ONLIST_VALIDATIONS
#define DLL2_LIST_NEXT(list,item) \
    (((item)->links[(list)->list_index].links_onlist==list)? \
     ((item)->links[(list)->list_index].links_next) : \
      dll2_error( __FILE__, __LINE__, "dll2_next wrong list" ))
#define DLL2_LIST_PREV(list,item) \
    (((item)->links[(list)->list_index].links_onlist==list)? \
     ((item)->links[(list)->list_index].links_prev) : \
      dll2_error( __FILE__, __LINE__, "dll2_prev wrong list" ))
/* note there are two versions of this macro (_T and _F); the reason
   is that when validations are turned off, some places need one default
   and others need the other. */
#define DLL2_NCMP_ONLIST(item,index,ptr) \
    ((item)->links[index].links_onlist != (ptr))
#define DLL2_NCMP_ONLIST_T(item,index,ptr) \
    DLL2_NCMP_ONLIST(item,index,ptr)
#define DLL2_NCMP_ONLIST_F(item,index,ptr) \
    DLL2_NCMP_ONLIST(item,index,ptr)
#else
#define DLL2_LIST_NEXT(list,item) \
    ((item)->links[(list)->list_index].links_next)
#define DLL2_LIST_PREV(list,item) \
    ((item)->links[(list)->list_index].links_prev)
#define DLL2_NCMP_ONLIST_T(item,index,ptr) 1
#define DLL2_NCMP_ONLIST_F(item,index,ptr) 0
#endif

#define DLL2_LINKS_OFFSET(item) \
    (((unsigned int)((item)->links)) - ((unsigned int)(item)))
#define DLL2_DIM(array) (sizeof(array) / sizeof(array[0]))

#if DLL2_MAGIC_VALIDATIONS
#define DLL2_LIST_ISINIT(list) \
    ((list)->list_magic == DLL2_LIST_MAGIC)
#define DLL2_LINKS_ISINIT(item,index) \
    ((item)->links[index].links_magic == DLL2_LINKS_MAGIC )
#define DLL2_INIT_LIST_MAGIC(list) \
    (list)->list_magic = DLL2_LIST_MAGIC
#define DLL2_INIT_LINKS_MAGIC(item,index) \
    (item)->links[index].links_magic = DLL2_LINKS_MAGIC;
#define DLL2_DEINIT_LIST_MAGIC(list) \
    (list)->list_magic = 0
#define DLL2_DEINIT_LINKS_MAGIC(item,index) \
    (item)->links[index].links_magic = 0
#define DLL2_LIST_VALIDATE_NOTINIT(list) \
    if ( DLL2_LIST_ISINIT(list) ) \
        (void) dll2_error( __FILE__, __LINE__, \
                           "dll2_list_init already initialized?" )
#define DLL2_LINKS_VALIDATE_NOTINIT(item,index) \
    if ( DLL2_LINKS_ISINIT(item,index) ) \
        (void) dll2_error( __FILE__, __LINE__, \
                           "dll2_item_init already initialized?" )
#else
#define DLL2_INIT_LIST_MAGIC(list) /* nothing */
#define DLL2_INIT_LINKS_MAGIC(item,index) /* nothing */
#define DLL2_DEINIT_LIST_MAGIC(list) /* nothing */
#define DLL2_DEINIT_LINKS_MAGIC(item,index) /* nothing */
#define DLL2_LIST_VALIDATE_NOTINIT(list) /* nothing */
#define DLL2_LINKS_VALIDATE_NOTINIT(item,index) /* nothing */
#endif

extern void dll2_list_add( DLL2_LIST * list, int links_offset,
                           void * item, char * file, int line );
extern void dll2_list_remove( DLL2_LIST * list, int links_offset,
                              void * item, char * file, int line );
extern void dll2_list_add_after( DLL2_LIST * list, int links_offset,
                                 void * existing, void * item,
                                 char * file, int line );
extern void dll2_list_add_before( DLL2_LIST * list, int links_offset,
                                  void * existing, void * item,
                                  char * file, int line );
/* this function only has a void * return type to eliminate certain
   warnings in the above macros which use the ternary ?: operator */
extern void * dll2_error( char * file, int line, char * format, ... );
