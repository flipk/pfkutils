/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
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
    PK_Semaphore( char * _name );
    ~PK_Semaphore( void );
    friend class PK_Semaphores; // only create thru PK_Semaphores class
    char * name;
public:
    LListLinks <PK_Semaphore> links[PK_SEM_NUMLISTS];
    char * get_name( void ) { return name; }
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
    static int hash_key( char * key ) { return string_hash( key ); }
    static bool hash_key_compare( PK_Semaphore * item, char * key ) {
        return ( strcmp( item->get_name(), key ) == 0 );
    }
};

class PK_Semaphores {
    pthread_mutex_t  mutex;
    LList     <PK_Semaphore,        PK_SEM_LIST>  list;
    LListHash <PK_Semaphore,  char *,
               PK_Semaphore_hash_1, PK_SEM_HASH>  hash;
    void   _lock( void ) { pthread_mutex_lock  ( &mutex ); }
    void _unlock( void ) { pthread_mutex_unlock( &mutex ); }
public:
    PK_Semaphores( void );
    ~PK_Semaphores( void );
    PK_Semaphore * create( char * name ) {
        PK_Semaphore * ret = new PK_Semaphore( name );
        _lock();
        list.add( ret );
        hash.add( ret );
        _unlock();
        return ret;
    }
    PK_Semaphore * lookup( char * name ) {
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
    PK_Semaphore * get_head( void ) { return list.get_head(); }
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
