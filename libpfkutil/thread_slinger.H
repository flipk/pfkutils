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

class thread_slinger_semaphore
{
    // a cheezy implementation of a counting semaphore.
    pthread_mutex_t  mutex;
    pthread_cond_t  waiter;
    int value;
    void   lock( void ) { pthread_mutex_lock  ( &mutex ); }
    void unlock( void ) { pthread_mutex_unlock( &mutex ); }
public:
    thread_slinger_semaphore(void);
    ~thread_slinger_semaphore(void);
    void init(int init_val) { value = init_val; }
    void give(void);
    // return false if timeout
    bool take(struct timespec * expire);
};

struct thread_slinger_message
{
    thread_slinger_message * next;
};

class _thread_slinger_queue
{
    pthread_mutex_t   mutex;
    pthread_cond_t  _waiter;
    pthread_cond_t  *waiter;
    // it's a little bit of a waste to create a sempahore
    // in every queue; when using the multiple-queue version
    // of _dequeue, only one of the semaphores is used. however,
    // this is cheaper IMHO than creating and destroying a semaphore
    // every time you enter and leave _dequeue(), which is the alternative.
    thread_slinger_semaphore  _waiter_sem;
    thread_slinger_semaphore * waiter_sem;
    thread_slinger_message * head;
    thread_slinger_message * tail;
    int count;
    void   lock( void ) { pthread_mutex_lock  ( &mutex ); }
    void unlock( void ) { pthread_mutex_unlock( &mutex ); }
    thread_slinger_message * __dequeue(void);
protected:
    _thread_slinger_queue(void);
    ~_thread_slinger_queue(void);
    void _enqueue(thread_slinger_message *);
    thread_slinger_message * _dequeue(int uSecs);
    int _get_count(void) { return count; }
    static thread_slinger_message * _dequeue(
        _thread_slinger_queue ** queues,
        int num_queues, int uSecs,
        int *which_queue=NULL);
};

template <class T>
class thread_slinger_queue : public _thread_slinger_queue
{
public:
    void enqueue(T * msg) { _enqueue(msg); }
    T * dequeue(int uSecs=0) { return (T *) _dequeue(uSecs); }
    int get_count(void) { return _get_count(); }
    // the priority of the queues relative to each other
    // is specified by the order of the queues array. first
    // queue is highest priority, etc.
    static T * dequeue(_thread_slinger_queue ** queues, int num_queues,
                       int uSecs=0, int *which_queue=NULL) {
        return (T *) _dequeue(queues,
                              num_queues, uSecs, which_queue);
    }
};

template <class T, int count>
class thread_slinger_pool
{
    thread_slinger_queue<T>  q;
    T * array;
public:
    thread_slinger_pool(void) {
        array = new T[count];
        for (int i = 0; i < count; i++)
            release(array + i);
    }
    ~thread_slinger_pool(void) {
        delete[] array;
    }
    T * alloc(int uSecs=0, bool grow=false) { return q.dequeue(uSecs); }
    void release(T * buf) { q.enqueue(buf); }
    int get_count(void) { return q.get_count(); }
};
