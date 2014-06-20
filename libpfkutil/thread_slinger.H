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

#include "LockWait.h"

class thread_slinger_semaphore : public Lockable, public Waitable
{
    int value;
public:
    thread_slinger_semaphore(void);
    ~thread_slinger_semaphore(void);
    void init(int init_val) { value = init_val; }
    void give(void);
    // return false if timeout
    bool take(struct timespec * expire);
};

/** base class for all user messages to go through thread_slinger,
 * no user-servicable parts inside */
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

/** a message queue of user objects, declare a derived type from
 * thread_slinger_message and specify that as T.
 * \param  T  the type to send through the queue, derived
 *          from thread_slinger_message base type */
template <class T>
class thread_slinger_queue : public _thread_slinger_queue
{
public:
    /** send a message to the receiver.
     * \param msg a user's message derived from thread_slinger_message */
    void enqueue(T * msg) { _enqueue(msg); }
    /** fetch a message from the sender.
     * \param uSecs  if >0, wait for that time and return NULL if no
     *         message; if <0, wait forever for a message; if 0,
     *         return NULL immediately if no message.
     * \return NULL if timeout, or a message pointer */
    T * dequeue(int uSecs=0) { return (T *) _dequeue(uSecs); }
    /** find out how many messages are currently queued */
    int get_count(void) { return _get_count(); }
    /** dequeue from a set of queues in priority order.
     * \param queues  the list of queues to check, the priority is
     *      specified by the order in this list (first queue in this
     *      list is checked first).
     * \param num_queues  the dimension of the queues array
     * \param uSecs  the time to wait for a message: >0 wait up to
     *       that many usecs; <0 wait forever; ==0 return NULL
     *       immediately if all queues empty.
     * \param which_queue  an optional pointer to an integer, which
     *       on return of a valid message pointer will indicate which
     *       queue this message came from (an index in queues array)
     * \return a message pointer or NULL if timeout */
    static T * dequeue(thread_slinger_queue<T> * queues[],
                       int num_queues,
                       int uSecs, int *which_queue=NULL) {
        return (T *) _dequeue((_thread_slinger_queue **) queues,
                              num_queues, uSecs, which_queue);
    }
};

/** a convenient buffer pool for storing objects of any kind.
 * \param T   the type of object being stored in the queue,
 *          derived from thread_slinger_message.
 * \param count  the number of objects to initially allocate
 *      when the pool is created. */
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
    /** allocate a buffer from the pool.
     * \param uSecs  how long to wait if the pool is empty:
     *          <0 means wait forever, ==0 means return NULL 
     *          immediately, >0 means wait that number of usecs.
     * \param grow   if true, allocate new buffers if pool is empty.
     * \return a buffer pointer, or NULL if empty and timeout */
    T * alloc(int uSecs=0, bool grow=false) { return q.dequeue(uSecs); }
    /** release a buffer pack into the pool
     * \param buf  the buffer pointer to release */
    void release(T * buf) { q.enqueue(buf); }
    /** fetch number of buffers currently in the pool */
    int get_count(void) { return q.get_count(); }
};

/** \mainpage threadslinger user's API documentation

This is the user's manual for the threadslinger API.

Interesting classes:

<ul>
<li> \ref thread_slinger_message
<li> \ref thread_slinger_queue
<li> \ref thread_slinger_pool
</ul>

\code

struct myMessage : public thread_slinger_message
{
    int a; // some field
    int b; // some other field
};

thread_slinger_pool<myMessage,5>  p;

typedef thread_slinger_queue<myMessage> myMsgQ;

myMsgQ q;
myMsgQ q2;

void * t3( void * dummy ) {
    uintptr_t  val = (uintptr_t) dummy;
    while (1) {
        myMessage * m = p.alloc(random()%5000);
        if (m) {
            if (val == 0)
                q.enqueue(m);
            else
                q2.enqueue(m);
            if (val == 0)
                printf("+");
            else
                printf("=");
        } else {
            if (val == 0)
                printf("-");
            else
                printf("_");
        }
        fflush(stdout);
        usleep(random()%30000);
    }
    return NULL;
}

void * t4(void * dummy) {
    myMsgQ * qs[2];
    qs[0] = &q;
    qs[1] = &q2;
    while (1) {
        int which;
        myMessage * m = myMsgQ::dequeue(qs,2,(int)(random()%1000),&which);
        if (m) {
            if (which == 0)
                printf(".");
            else
                printf(",");
            p.release(m);
        } else {
            printf("!");
        }
        fflush(stdout);
        usleep(random()%10000);
    }
    return NULL;
}

int main() {
    pthread_t id;
    pthread_create( &id, NULL, t3, (void*) 0 );
    pthread_create( &id, NULL, t3, (void*) 1 );
    pthread_create( &id, NULL, t4, (void*) 0 );
    pthread_join(id,NULL);
    return 0;
}

\endcode


*/
