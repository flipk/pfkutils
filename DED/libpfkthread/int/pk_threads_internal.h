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

#include "stringhash.h"

/** \cond INTERNAL */

class PK_Thread_hash_1 {
public:
    static int hash_key( const PK_Thread * item ) { 
        return hash_key( item->get_id() );
    }
    static int hash_key( const pthread_t key ) {
        // this hash algorithm assumes that a pthread_t is a pointer
        // value that happens to be 4-byte aligned -- meaning that the
        // bottom two bits are zero and thus not useful in the hash.
        return (int) ((unsigned long) key >> 2);
    }
    static bool hash_key_compare( const PK_Thread * item,
                                  const pthread_t key ) {
        return ( item->get_id() == key );
    }
};

class PK_Thread_hash_2 {
public:
    static int hash_key( const PK_Thread * item ) { 
        return hash_key( item->get_name() );
    }
    static int hash_key( const char * key ) { return string_hash( key ); }
    static bool hash_key_compare( const PK_Thread * item, const char * key ) {
        return ( strcmp( item->get_name(), key ) == 0 );
    }
};

class PK_Thread_List {
    LList     <PK_Thread,        PK_THREAD_LIST    >  list;
    LListHash <PK_Thread,  pthread_t,
               PK_Thread_hash_1, PK_THREAD_IDHASH  >  idhash;
    LListHash <PK_Thread,  const char *,
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
    PK_Thread * find_name( const char * n ) {
        PK_Thread * ret;
        _lock();
        ret = namehash.find( n );
        _unlock();
        return ret;
    }
    int get_cnt( void ) const {
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
/** \endcond */
