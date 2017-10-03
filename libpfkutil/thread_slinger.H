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

#include <pthread.h>
#include <inttypes.h>

struct thread_slinger_message
{
    thread_slinger_message * next;
};

class _thread_slinger_queue
{
    pthread_mutex_t   mutex;
    pthread_cond_t   waiter;
    thread_slinger_message * head;
    thread_slinger_message * tail;
    int count;
    void   _lock( void ) { pthread_mutex_lock  ( &mutex ); }
    void _unlock( void ) { pthread_mutex_unlock( &mutex ); }
protected:
    _thread_slinger_queue(void);
    ~_thread_slinger_queue(void);
    void _enqueue(thread_slinger_message *);
    thread_slinger_message * _dequeue(int uSecs);
    int _get_count(void) { return count; }
};

template <class T>
class thread_slinger_queue : public _thread_slinger_queue
{
public:
    void enqueue(T * msg) { _enqueue(msg); }
    T * dequeue(int uSecs=0) { return (T *) _dequeue(uSecs); }
    int get_count(void) { return _get_count(); }
};

template <class T, int count>
class thread_slinger_pool
{
    thread_slinger_queue<T>  q;
public:
    thread_slinger_pool(void) {
        T * array = new T[count];
        for (int i = 0; i < count; i++)
            release(array + i);
    }
    ~thread_slinger_pool(void) { /*cleanup?*/ }
    T * alloc(int uSecs=0, bool grow=false) { return q.dequeue(uSecs); }
    void release(T * buf) { q.enqueue(buf); }
    int get_count(void) { return q.get_count(); }
};
