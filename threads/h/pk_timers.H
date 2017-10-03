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

#ifndef __TIMERS_H__
#define __TIMERS_H__

#include "dll2.H"
#include "pk_msg_int.H"
#include "pk_messages.H"

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
    int   global_time;
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
    int   get_time( void ) { return global_time; }
};

extern PK_Timer_Manager * PK_Timers_global;

#endif /* __TIMERS_H__ */
