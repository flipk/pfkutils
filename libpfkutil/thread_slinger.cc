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

_thread_slinger_queue :: _thread_slinger_queue(void)
{
    pthread_mutexattr_t  mattr;
    pthread_condattr_t   cattr;

    head = tail = NULL;
    count = 0;

    pthread_mutexattr_init( &mattr );
    pthread_mutex_init( &mutex, &mattr );
    pthread_mutexattr_destroy( &mattr );

    pthread_condattr_init( &cattr );
    pthread_cond_init( &waiter, &cattr );
    pthread_condattr_destroy( &cattr );
}

_thread_slinger_queue :: ~_thread_slinger_queue(void)
{
    // cleanup the queue?
    pthread_mutex_destroy( &mutex );
    pthread_cond_destroy( &waiter );
}

void 
_thread_slinger_queue :: _enqueue(thread_slinger_message * pMsg)
{
    pMsg->next = NULL;
    _lock();
    if (tail)
    {
        tail->next = pMsg;
        tail = pMsg;
    }
    else
        head = tail = pMsg;
    count++;
    _unlock();
    pthread_cond_signal(&waiter);
}

thread_slinger_message *
_thread_slinger_queue :: _dequeue(int uSecs)
{
    thread_slinger_message * pMsg = NULL;
    struct timespec abstime;
    abstime.tv_sec = 0;
    _lock();
    if (head == NULL)
    {
        if (uSecs == 0)
        {
            _unlock();
            return NULL;
        }
        while (head == NULL)
        {
            if (uSecs == -1)
            {
                pthread_cond_wait( &waiter, &mutex );
            }
            else
            {
                if (abstime.tv_sec == 0)
                {
                    clock_gettime( CLOCK_REALTIME, &abstime );
                    abstime.tv_sec  += uSecs / 1000000;
                    abstime.tv_nsec += (uSecs % 1000000) * 1000;
                    if ( abstime.tv_nsec > 1000000000 )
                    {
                        abstime.tv_nsec -= 1000000000;
                        abstime.tv_sec ++;
                    }
                }
                int ret = pthread_cond_timedwait( &waiter, &mutex, &abstime );
                if ( ret != 0 )
                    break;
            }
        }
    }
    if (head != NULL)
    {
        pMsg = head;
        head = head->next;
        if (head == NULL)
            tail = NULL;
        pMsg->next = NULL;
        count--;
    }
    _unlock();
    return pMsg;
}
