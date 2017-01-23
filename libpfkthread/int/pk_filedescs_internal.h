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

#ifndef __PK_FILEDESCS_INTERNAL_H__
#define __PK_FILEDESCS_INTERNAL_H__

#include "dll2.h"

/** \cond INTERNAL */
enum {
    PK_FD_Queue,
    PK_FD_Hash,
    PK_FD_NumLists
};

struct PK_File_Descriptor {
    LListLinks<PK_File_Descriptor> links[PK_FD_NumLists];
    int pkfdid;
    int fd;
    PK_FD_RW rw;
    int qid;
    void * obj;
};

class PK_File_Descriptor_Hash_Comparator {
public:
    static int hash_key( PK_File_Descriptor * item ) { return item->pkfdid; }
    static int hash_key( const int key ) { return key; }
    static bool hash_key_compare( PK_File_Descriptor * item, const int key ) {
        return (item->pkfdid == key); 
    }
};

class PK_File_Descriptor_List {
    LList <PK_File_Descriptor,PK_FD_Queue>  q;
    LListHash <PK_File_Descriptor, int,
               PK_File_Descriptor_Hash_Comparator,
               PK_FD_Hash>  hash;
public:
    PK_File_Descriptor_List( void ) { /* nothing */ }
    ~PK_File_Descriptor_List( void );
    PK_File_Descriptor * find( int pkfdid ) { return hash.find(pkfdid); }
    void add( PK_File_Descriptor * d ) { q.add(d); hash.add(d); }
    void remove( PK_File_Descriptor * d ) { q.remove(d); hash.remove(d); }
    PK_File_Descriptor * get_head(void) { return q.get_head(); }
    PK_File_Descriptor * get_next(PK_File_Descriptor * x) {
        return q.get_next(x); }
};

class PK_File_Descriptor_Thread : public PK_Thread {
    /*virtual*/ void entry( void );
    PK_File_Descriptor_Manager * mgr;
public:
    PK_File_Descriptor_Thread( PK_File_Descriptor_Manager * _mgr );
    ~PK_File_Descriptor_Thread( void ); // ?
    void stop( void ); // used by PK_Threads to shut down threading
};
/** \endcond */

#endif /* __PK_FILEDESCS_INTERNAL_H__ */
