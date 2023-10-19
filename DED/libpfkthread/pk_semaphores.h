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

#ifndef __SEMAPHORES_H__
#define __SEMAPHORES_H__

#include "dll2.h"

#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#include "stringhash.h"

enum { PK_SEM_LIST, PK_SEM_HASH, PK_SEM_NUMLISTS };

/** a semaphore.
 * this semaphore is a counting semaphore: the \ref take method attempts
 * to subtract 1 from the value, blocking (either for a timeout period,
 * or forever) if the value is at zero, and the \ref give method
 * adds one, resuming a blocked \ref take if one exists.
 * \note the initial value of the semaphore is 1, which is suitable for
 *       use as a mutex (a \ref take and \ref give around critical code
 *       regions. if however a thread-to-thread signal is required, an
 *       initial \ref take should be perform immediately after creation
 *       to drop the value of the semaphore to 0, so that it is now suitable
 *       for use as a signaling device. */
class PK_Semaphore {
    pthread_mutex_t  mutex;
    pthread_cond_t   cond;
    int value;
    int waiters;
    void   _lock( void ) { pthread_mutex_lock  ( &mutex ); }
    void _unlock( void ) { pthread_mutex_unlock( &mutex ); }
    PK_Semaphore( const char * _name );
    ~PK_Semaphore( void );
    /** \cond INTERNAL */
    friend class PK_Semaphores; // only create thru PK_Semaphores class
    /** \endcond */
    char * name;
public:
    /** \cond INTERNAL */
    LListLinks <PK_Semaphore> links[PK_SEM_NUMLISTS];
    /** \endcond */
    /** return the name string of the semaphore
     * \return character string representing the name of the semaphore */
    const char * get_name( void ) { return name; }
    /** attempt to acquire the semaphore (decrement value).
     * if the current value of the semaphore is zero, the calling
     * thread will block for up to the given number of timer ticks
     * waiting for a \ref give.
     * \note if multiple processes are blocked waiting on the same
     *       semaphore, they are queued in FIFO order: a \ref give will 
     *       wake up the next waiter in the queue.
     * \param ticks  the number of timer ticks to wait for a \ref give. <ul>
     *              <li> -1 : wait forever
     *              <li> 0 : attempt to take, and return immediately
     *                   whether success or failure
     *              <li> >0 : if not available, block for up to the
     *                   specified number of ticks waiting for a \ref give.
     *              </ul>
     * \return <ul> <li> true if the semaphore was taken (value was
     *         decremented) or <li> false if there was a timeout </ul> */
    bool take( int ticks );
    /** acquire the semaphore (decrement the value).
     * this call is equivialent to \ref take(-1), to wait forever if the
     * semaphore is not available. */
    void take( void ) { (void) take( -1 ); }
    /** release the semaphore (increment the value).
     * this call increments the value of the semaphore. if there is a 
     * waiter blocked in \ref take, it will be resumed.
     * \note if multiple processes are blocked waiting on the same
     *       semaphore, they are queued in FIFO order: a \ref give will 
     *       wake up the next waiter in the queue. */
 void give( void );
};

/** \cond INTERNAL */
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
/** \endcond */

#endif /* __SEMAPHORES_H__ */
