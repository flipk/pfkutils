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

#ifndef __TIMERS_H__
#define __TIMERS_H__

#include "dll2.h"
#include "pk_messages.h"

#include "pfkutils_config.h"
#if HAVE_INTTYPES_H
#include <inttypes.h>
#endif

class PK_Timer;
class PK_Timer_List;

class PK_Timeout_Obj {
    bool condcreated;
public:
    pthread_cond_t  cond;
    bool timedout;
    PK_Timeout_Obj( void )
    {
        pthread_cond_init( &cond, NULL );
        timedout = false;
        condcreated = true;
    }
    PK_Timeout_Obj( pthread_cond_t  _cond )
    {
        cond = _cond;
        timedout = false;
        condcreated = false;
    }
    ~PK_Timeout_Obj( void )
    {
        if ( condcreated )
            pthread_cond_destroy( &cond );
    }
};

/** \cond INTERNAL */
class PK_Timer_Manager {
    pthread_mutex_t  mutex;
    PK_Timer_List  * timers;
    int fds[2]; // pipe
    pthread_t th1;
    pthread_t th2;
    void _thread1( void );
    void _thread2( void );
    static void * timer_thread1( void * arg );
    static void * timer_thread2( void * arg );
    void   _lock( void ) { pthread_mutex_lock  ( &mutex ); }
    void _unlock( void ) { pthread_mutex_unlock( &mutex ); }
    int  _create( PK_Timer *, int ticks );
    void  expire( PK_Timer * );
    int ticks_per_second;
    uint64_t global_time;
public:
    PK_Timer_Manager( int tps );
    ~PK_Timer_Manager( void );
// returns a timer id
    int   create( int ticks, int qid, pk_msg_int * msg );
    int   create( int ticks, void (*func)(void *), void * arg );
    int   create( int ticks, PK_Timeout_Obj * pkto );
// returns true, msg ptr, and ticks remaining if timer was still running
    bool  cancel( int tid, pk_msg_int **, int * ticks_remaining ); 
    bool  cancel( int tid ) { return cancel( tid, NULL, NULL ); }
//
    int   tps( void ) { return ticks_per_second; }
    void  sleep( int ticks );
    uint64_t get_time( void ) { return global_time; }
};

extern PK_Timer_Manager * PK_Timers_global;
/** \endcond */

#endif /* __TIMERS_H__ */
