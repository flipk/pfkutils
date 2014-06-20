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

#include <errno.h>
#include <string.h>
#include <stdio.h>

#include "thread_slinger.H"

thread_slinger_semaphore :: thread_slinger_semaphore(void)
{
    value = 0;
}

thread_slinger_semaphore :: ~thread_slinger_semaphore(void)
{
}

void
thread_slinger_semaphore :: give(void)
{
    {
        Lock  lock(this);
        value ++;
    }
    waiterSignal();
}

// return false if timeout; if expire==NULL, wait forever.
bool
thread_slinger_semaphore :: take(struct timespec * expire)
{
    Waiter waiter(this);
    while (value <= 0)
    {
        if (expire)
        {
            if (waiter.wait(expire) == false)
                return false;
        }
        else
            waiter.wait();
    }
    value --;
    return true;
}

//

static inline struct timespec *
setup_abstime(int uSecs, struct timespec *abstime)
{
    if (uSecs < 0)
        return NULL;
    clock_gettime( CLOCK_REALTIME, abstime );
    abstime->tv_sec  +=  uSecs / 1000000;
    abstime->tv_nsec += (uSecs % 1000000) * 1000;
    if ( abstime->tv_nsec > 1000000000 )
    {
        abstime->tv_nsec -= 1000000000;
        abstime->tv_sec ++;
    }
    return abstime;
}

_thread_slinger_queue :: _thread_slinger_queue(void)
{
    pthread_mutexattr_t  mattr;
    pthread_condattr_t   cattr;
    head = tail = NULL;
    count = 0;
    waiter = NULL;
    waiter_sem = NULL;
    pthread_mutexattr_init( &mattr );
    pthread_mutex_init( &mutex, &mattr );
    pthread_mutexattr_destroy( &mattr );
    pthread_condattr_init( &cattr );
    pthread_cond_init( &_waiter, &cattr );
    pthread_condattr_destroy( &cattr );
}

_thread_slinger_queue :: ~_thread_slinger_queue(void)
{
    // cleanup the queue?
    pthread_mutex_destroy( &mutex );
    pthread_cond_destroy( &_waiter );
}

// the mutex must be locked before calling this.
thread_slinger_message *
_thread_slinger_queue :: __dequeue(void)
{
    thread_slinger_message * pMsg = NULL;
    if (head != NULL)
    {
        pMsg = head;
        head = head->next;
        if (head == NULL)
            tail = NULL;
        pMsg->next = NULL;
        count--;
    }
    return pMsg;
}

void 
_thread_slinger_queue :: _enqueue(thread_slinger_message * pMsg)
{
    pMsg->next = NULL;
    lock();
    if (tail)
    {
        tail->next = pMsg;
        tail = pMsg;
    }
    else
        head = tail = pMsg;
    count++;
    pthread_cond_t * w = waiter;
    thread_slinger_semaphore * sem = waiter_sem;
    unlock();
    // a given queue will have a non-null waiter 
    // iff the receiver thread is blocked in the single-queue
    // _dequeue.  it will have a non-null sem
    // iff the receiver thread is blocked in the multi-queue
    // _dequeue.
    if (w)
        pthread_cond_signal(w);
    if (sem)
        sem->give();
}

thread_slinger_message *
_thread_slinger_queue :: _dequeue(int uSecs)
{
    thread_slinger_message * pMsg = NULL;
    struct timespec abstime;
    abstime.tv_sec = 0;
    lock();
    if (head == NULL)
    {
        if (uSecs == 0)
        {
            unlock();
            return NULL;
        }
        while (head == NULL)
        {
            if (uSecs == -1)
            {
                waiter = &_waiter;
                pthread_cond_wait( waiter, &mutex );
                waiter = NULL;
            }
            else
            {
                if (abstime.tv_sec == 0)
                    setup_abstime(uSecs, &abstime);
                waiter = &_waiter;
                int ret = pthread_cond_timedwait( waiter, &mutex, &abstime );
                waiter = NULL;
                if ( ret != 0 )
                    break;
            }
        }
    }
    pMsg = __dequeue();
    unlock();
    return pMsg;
}

//static
thread_slinger_message *
_thread_slinger_queue :: _dequeue(_thread_slinger_queue ** queues,
                                  int num_queues, int uSecs,
                                  int *which_queue)
{
    thread_slinger_message * pMsg = NULL;
    thread_slinger_semaphore * sem = &queues[0]->_waiter_sem;
    int ind;
    struct timespec abstime;
    struct timespec * pTime = setup_abstime(uSecs, &abstime);
    // add sem to all queues
    for (ind = 0; ind < num_queues; ind++)
        queues[ind]->waiter_sem = sem;
    sem->init(0);
    // wait for msg
    do {
        for (ind = 0; ind < num_queues; ind++)
        {
            queues[ind]->lock();
            pMsg = queues[ind]->__dequeue();
            queues[ind]->unlock();
            if (pMsg)
            {
                if (which_queue)
                    *which_queue = ind;
                break;
            }
        }
        if (pMsg == NULL)
            if (sem->take(pTime) == false)
                break;
    } while (pMsg == NULL);
    // remove sem from all queues
    for (ind = 0; ind < num_queues; ind++)
        queues[ind]->waiter_sem = NULL;
    return pMsg;
}
