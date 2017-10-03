
/*
    This file is part of the "pfkutils" tools written by Phil Knaack
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

#include "stringhash.H"

class PK_Thread_hash_1 {
public:
    static int hash_key( PK_Thread * item ) { 
        return hash_key( item->get_id() );
    }
    static int hash_key( pthread_t key ) {
        // this hash algorithm assumes that a pthread_t is a pointer
        // value that happens to be 4-byte aligned -- meaning that the
        // bottom two bits are zero and thus not useful in the hash.
        return (int) ((unsigned long) key >> 2);
    }
    static bool hash_key_compare( PK_Thread * item, pthread_t key ) {
        return ( item->get_id() == key );
    }
};

class PK_Thread_hash_2 {
public:
    static int hash_key( PK_Thread * item ) { 
        return hash_key( item->get_name() );
    }
    static int hash_key( char * key ) { return string_hash( key ); }
    static bool hash_key_compare( PK_Thread * item, char * key ) {
        return ( strcmp( item->get_name(), key ) == 0 );
    }
};

class PK_Thread_List {
    LList     <PK_Thread,        PK_THREAD_LIST    >  list;
    LListHash <PK_Thread,  pthread_t,
               PK_Thread_hash_1, PK_THREAD_IDHASH  >  idhash;
    LListHash <PK_Thread,  char *,
               PK_Thread_hash_2, PK_THREAD_NAMEHASH>  namehash;
    pthread_mutex_t  mutex;
    void   _lock( void ) { pthread_mutex_lock  ( &mutex ); }
    void _unlock( void ) { pthread_mutex_unlock( &mutex ); }
public:
    PK_Thread_List( void ) {
        pthread_mutex_init( &mutex, NULL );
    }
    ~PK_Thread_List( void ) {
        pthread_mutex_destroy( &mutex );
    }
    void add( PK_Thread * t ) {
        _lock();
        list.add( t );
        idhash.add( t );
        namehash.add( t ); 
        _unlock();
    }
    void remove( PK_Thread * t ) {
        _lock();
        list.remove( t );
        idhash.remove( t );
        namehash.remove( t );
        _unlock();
    }
    void unhash_name( PK_Thread * t ) {
        _lock(); namehash.remove( t );
    }
    void rehash_name( PK_Thread * t ) {
        namehash.add( t ); _unlock();
    }
    PK_Thread * find_id( pthread_t id ) {
        PK_Thread * ret;
        _lock();
        ret = idhash.find( id );
        _unlock();
        return ret;
    }
    PK_Thread * find_name( char * n ) {
        PK_Thread * ret;
        _lock();
        ret = namehash.find( n );
        _unlock();
        return ret;
    }
    int get_cnt( void ) {
        return list.get_cnt();
    }
    PK_Thread * get_head( void ) {
        return list.get_head();
    }
    // xxx : iteration over this list needs to be 
    //       atomic for the whole list, not just at each step
    PK_Thread * get_next( PK_Thread * t ) {
        PK_Thread * ret;
        _lock();
        ret = list.get_next( t );
        _unlock();
        return ret;
    }
};
