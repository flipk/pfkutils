
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

#include "pk_timers.H"
#include "pk_timers_internal.H"

#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>

PK_Timer_Manager * PK_Timers_global;

PK_Timer_Manager :: PK_Timer_Manager( int _tps )
{
    if ( PK_Timers_global )
        kill(0,6);

    ticks_per_second = _tps;
    timers = new PK_Timer_List;

    pthread_mutex_init( &mutex, NULL );

    pipe( fds );

    pthread_create( &th1, NULL, timer_thread1, (void*) this );
    pthread_create( &th2, NULL, timer_thread2, (void*) this );

    pthread_detach( th1 );
    pthread_detach( th2 );

    global_time = 0;
    PK_Timers_global = this;
}

PK_Timer_Manager :: ~PK_Timer_Manager( void )
{
    pthread_cancel( th1 );
    pthread_cancel( th2 );
    PK_Timers_global = NULL;

    PK_Timer * t;
    while ((t = timers->get_head()) != NULL)
    {
        fprintf( stderr, "while deleting timer manager: "
                 "deleting stale timer of type %d (%#x)\n",
                 t->type,
                 t->type == PK_TIMER_MSG ? t->u.msg.msg->type : 0 );
        timers->remove( t );
        delete t;
    }

    pthread_mutex_destroy( &mutex );
    delete timers;

    // why does closing fd[1] first, make cygwin work,
    // where closing fd[0] first makes it hang?

    close( fds[1] );
    close( fds[0] );
}

int
PK_Timer_Manager :: _create( PK_Timer * nt, int ticks )
{
    int tid;
    PK_Timer * t = NULL;
    _lock();
    do {
        tid = random() & 0x7fffffff;
        if ( tid == 0 || tid == -1 )
            continue;
        t = timers->find( tid );
    } while ( t != NULL );
    nt->tid = tid;
//    nt->set_time = global_time;  // only useful for debugging
    nt->ordered_queue_key = ticks;
    nt->expire_time = global_time + nt->ordered_queue_key;
    timers->add( nt );
    _unlock();
//    printf( "timer %d created\n", tid );
    return tid;
}

int
PK_Timer_Manager :: create( int ticks, int qid, pk_msg_int * msg )
{
    PK_Timer * nt = new PK_Timer;

    nt->type = PK_TIMER_MSG;
    nt->u.msg.qid = qid;
    nt->u.msg.msg = msg;

    return _create( nt, ticks );
}

int
PK_Timer_Manager :: create( int ticks, void (*func)(void *), void * arg )
{
    PK_Timer * nt = new PK_Timer;

    nt->type = PK_TIMER_FUNC;
    nt->u.func.func = func;
    nt->u.func.arg = arg;

    return _create( nt, ticks );
}

int
PK_Timer_Manager :: create( int ticks, PK_Timeout_Obj * pkto )
{
    PK_Timer * nt = new PK_Timer;

    nt->type = PK_TIMER_COND;
    nt->u.cond.pkto = pkto;

    return _create( nt, ticks );
}

bool  // return true, msg ptr, and ticks remaining if timer still running
PK_Timer_Manager :: cancel( int tid,
                            pk_msg_int ** msgp,
                            int * ticks_remaining )
{
    PK_Timer * t;

    _lock();
    t = timers->find( tid );
    if ( t )
        timers->remove( t );
    _unlock();

//    printf( "cancel timer %d: %s\n", tid, t ? "found" : "not found" );

    if ( t )
    {
        if ( t->type == PK_TIMER_MSG )
        {
            if ( msgp )
                *msgp = t->u.msg.msg;
            else
                delete t->u.msg.msg;
        }
        if ( ticks_remaining )
        {
            *ticks_remaining = t->expire_time - global_time;
            if ( *ticks_remaining < 0 )
                *ticks_remaining = 0;
        }
        delete t;
        return true;
    }

    return false;
}

void
PK_Timer_Manager :: sleep( int ticks )
{
    PK_Timeout_Obj    pkto;

    (void) create( ticks, &pkto );

    pthread_mutex_t   mut;
    pthread_mutex_init( &mut, NULL );
    pthread_mutex_lock( &mut );
    pthread_cond_wait( &pkto.cond, &mut );
    pthread_mutex_destroy( &mut );
}

//static
void *
PK_Timer_Manager :: timer_thread1( void * arg )
{
    PK_Timer_Manager * pkts = (PK_Timer_Manager*) arg;
    pkts->_thread1();
    return NULL;
}

//static
void *
PK_Timer_Manager :: timer_thread2( void * arg )
{
    PK_Timer_Manager * pkts = (PK_Timer_Manager*) arg;
    pkts->_thread2();
    return NULL;
}

void
PK_Timer_Manager :: _thread1( void )
{
    char c = 1;
    int delay = 1000000 / ticks_per_second;
    while ( 1 )
    {
        usleep( delay );
        pthread_testcancel();
        write( fds[1], &c, 1 );
    }
}

void
PK_Timer_Manager :: expire( PK_Timer * t )
{
//    printf( "expiring timer %d\n", t->tid );
    switch ( t->type )
    {
    case PK_TIMER_MSG:
        PK_Messages_global->send( t->u.msg.qid, t->u.msg.msg );
        break;
    case PK_TIMER_FUNC:
        t->u.func.func( t->u.func.arg );
        break;
    case PK_TIMER_COND:
        t->u.cond.pkto->timedout = true;
        pthread_cond_signal( &t->u.cond.pkto->cond );
        break;
    }
    delete t;
}

void
PK_Timer_Manager :: _thread2( void )
{
    char c;
    PK_Timer * t;
    while ( 1 )
    {
        if (read( fds[0], &c, 1 ) <= 0)
            break;

        pthread_testcancel();
        global_time++;
        do {
            _lock();
            t = timers->get_head();
            if ( t )
            {
                if ( t->ordered_queue_key <= 0 )
                    timers->remove( t );
                else
                {
                    t->ordered_queue_key --;
                    t = NULL;
                }
            }
            _unlock();
            if ( t )
                expire( t );
        } while ( t );
    }
}
