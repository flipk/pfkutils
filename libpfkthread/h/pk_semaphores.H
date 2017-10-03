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

#ifndef __SEMAPHORES_H__
#define __SEMAPHORES_H__

#include "dll2.H"

#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#include "stringhash.H"

enum { PK_SEM_LIST, PK_SEM_HASH, PK_SEM_NUMLISTS };

class PK_Semaphore {
    pthread_mutex_t  mutex;
    pthread_cond_t   cond;
    int value;
    int waiters;
    void   _lock( void ) { pthread_mutex_lock  ( &mutex ); }
    void _unlock( void ) { pthread_mutex_unlock( &mutex ); }
    PK_Semaphore( const char * _name );
    ~PK_Semaphore( void );
    friend class PK_Semaphores; // only create thru PK_Semaphores class
    char * name;
public:
    LListLinks <PK_Semaphore> links[PK_SEM_NUMLISTS];
    const char * get_name( void ) { return name; }
    // false if timeout
    bool take( int ticks );
    void take( void ) { (void) take( -1 ); }
    void give( void );
};

class PK_Semaphore_hash_1 {
public:
    static int hash_key( PK_Semaphore * item ) {
        return hash_key( item->get_name() );
    }
    static int hash_key( const char * key ) { return string_hash( key ); }
    static bool hash_key_compare( PK_Semaphore * item,
                                  const char * key ) {
        return ( strcmp( item->get_name(), key ) == 0 );
    }
};

class PK_Semaphores {
    pthread_mutex_t  mutex;
    LList     <PK_Semaphore,        PK_SEM_LIST>  list;
    LListHash <PK_Semaphore,  const char *,
               PK_Semaphore_hash_1, PK_SEM_HASH>  hash;
    void   _lock( void ) { pthread_mutex_lock  ( &mutex ); }
    void _unlock( void ) { pthread_mutex_unlock( &mutex ); }
public:
    PK_Semaphores( void );
    ~PK_Semaphores( void );
    PK_Semaphore * create( const char * name ) {
        PK_Semaphore * ret = new PK_Semaphore( name );
        _lock();
        list.add( ret );
        hash.add( ret );
        _unlock();
        return ret;
    }
    PK_Semaphore * lookup( const char * name ) {
        PK_Semaphore * ret;
        _lock();
        ret = hash.find( name );
        _unlock();
        return ret;
    }
    void destroy( PK_Semaphore * s ) {
        _lock();
        list.remove( s );
        hash.remove( s );
        _unlock();
        delete s;
    }
    PK_Semaphore * get_head( void ) const { return list.get_head(); }
    PK_Semaphore * get_next( PK_Semaphore * s ) {
        PK_Semaphore * ret;
        _lock();
        ret = list.get_next( s );
        _unlock();
        return ret;
    }
};

extern PK_Semaphores * PK_Semaphores_global;

#endif /* __SEMAPHORES_H__ */
