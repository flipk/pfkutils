
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

#ifndef __PK_FILEDESCS_INTERNAL_H__
#define __PK_FILEDESCS_INTERNAL_H__

#include "dll2.H"

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
    static int hash_key( int key ) { return key; }
    static bool hash_key_compare( PK_File_Descriptor * item, int key ) {
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

#endif /* __PK_FILEDESCS_INTERNAL_H__ */
