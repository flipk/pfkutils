/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

/**
 * \file pk_threads.h
 * \brief definition of PK_Threads and PK_Thread types
 */

/**
 \mainpage PK Threads library

This is the documentation for the PK_Threads library. 

 \section pkthreads_copyright  GPL version 2 - Copyright

<pre>
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
</pre>

 \section pkthreads_overview  Overview

The PK_Threads library provides the following features:

<ul>
<li> threads can create message queues using textual names that other threads
     can lookup on the fly, or by integer identifiers.
<li> threads can listen on multiple message queues at once, and can select
     their relative priority on the fly.
<li> messages are defined as C++ classes derived from a basic message type.
<li> message queues are zero-copy : message allocated and populated in
     the sender and the same memory is accessed and released by the
     receiver.
<li> counting semaphores, useful for critical-region mutual exclusion as
     well as for worker-initiator signaling.
<li> semaphores also created and looked up by textual name or integer
     identifier.
<li> threads can create an arbitrary number of timers which count down
     according to a process-wide time tick.
<li> timers may perform one of several actions when they expire: 
    <ul>
    <li> enqueue a message to a message queue
    <li> call a function
    <li> signal a pthread_cond_t
    </ul>
<li> timers have an associated integer identifier which can be used to
     cancel the timer before it expires; if a message is associated with
     the timer, the message can be returned to the caller.
<li> file descriptor integration with message queues: a thread may
     register to receive message indications concerning activity on an
     arbitrary number of file descriptors, and attach context object
     pointers to those registrations.
<li> threads are defined as a C++ class derived from a PK_Thread base
     class with virtual \a entry method as the "main" for the thread,
     allowing per-thread data to be stored in the class and allowing
     an arbitrary number of instances of a thread class, each with its
     own private data.
</ul>

 \section pkthreads_samplecode

This first application demonstrates message queues and some basic timer
facilities.

\ref pkthreads_sample1

\page pkthreads_sample1  PK_Threads sample program 1

\code

#include <stdio.h>
#include <unistd.h>
#include "pk_threads.h"

PkMsgIntDef( testmsg, 4,
             int a;
    );

class test1 : public PK_Thread
{
    void entry( void )
    {
        int qids[2], qi;
        qids[0] = msg_lookup( "q %d", 1 );
        qids[1] = msg_lookup( "q %d", 2 );
        union {
            pk_msg_int * m;
            testmsg * tm;
        } m;
        m.m = msg_recv( 2, qids, &qi, -1 );
        printf( "test1 got msg on q %d : type = %d a = %d (expected q=1 a=4)\n",
                qi, m.m->type, m.tm->a );
        delete m.m;
        m.m = msg_recv( 2, qids, &qi, -1 );
        printf( "test1 got msg on q %d : type = %d a = %d (expected q=0 a=5)\n",
                qi, m.m->type, m.tm->a );
        delete m.m;
        m.m = msg_recv( 2, qids, &qi, 5 );
        printf( "received ptr = %#lx (expected null)\n", (unsigned long)m.m);
    }
public:
    test1( void )
    {
        set_name( "test %d", 1 ); resume();
    }
};

class test2 : public PK_Thread
{
    void entry( void )
    {
        int qids[2];
        qids[0] = msg_lookup( "q %d", 1 );
        qids[1] = msg_lookup( "q %d", 2 );
        testmsg * m;
        m = new testmsg;
        m->a = 5;
        sleep( 1 );
        timer_create( 5, qids[0], m );
        m = new testmsg;
        m->a = 4;
        sleep( 1 );
        msg_send( qids[1], m );
    }
public:
    test2( void )
    {
        set_name( "test %d", 2 );
        resume();
    }
};

class test3 : public PK_Thread
{
    void entry( void )
    {
        int i;
        printf( "about to sleep\n" );
        for ( i = 10; i >= 0; i-- )
        {
            printf( "%d ", i );
            fflush( stdout );
            sleep( 1 );
        }
        printf( "\ndone sleeping, creating 2 test threads\n" );
        new test1;
        new test2;
    }
public:
    test3( void ) {
        resume();
    }
};

int main()
{
    new PK_Threads( 10 );
    int qid1 = PK_Thread::msg_create( "q %d", 1 );
    int qid2 = PK_Thread::msg_create( "q %d", 2 );
    new test3;
    th->run();
    PK_Thread::msg_destroy( qid1 );
    PK_Thread::msg_destroy( qid2 );
    delete th;
    return 0;
}
\endcode

 */

#ifndef __THREADS_H__
#define __THREADS_H__

#include "dll2.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "pk_messages.h"
#include "pk_messages_ext.h"
#include "pk_semaphores.h"
#include "pk_timers.h"
#include "pk_filedescs.h"

class PK_Thread;
class PK_Thread_List;

/** global singleton threads management object.
 * an object of this type should be declared in your main function.
 * then construct the \ref PK_Thread derived objects, then call 
 * \ref PK_Threads::run.  \ref PK_Threads::run will block while any
 * threads exist. When the last thread exits (returns from 
 * it's PK_Thread::entry method), PK_Threads::run will then return.
 * sample code:
 * \code
 * class MyThread1 : public PK_Thread { ... }
 * class MyThread2 : public PK_Thread { ... }
 * int main() {
 *    PK_Threads   th(TICKS_PER_SECOND);
 *    new MyThread1;
 *    new MyThread2;
 *    th.run();
 *    return 0;
 * }
 * \endcode
 */
class PK_Threads {
    PK_Thread_List * thread_list;
    pthread_cond_t  runner_wakeup;
    bool running;
    friend class PK_Thread;
    // following only called by PK_Thread base class
    void add( PK_Thread * t );
    void remove( PK_Thread * t );
    void unhash_name( PK_Thread * t );  // locks list
    void rehash_name( PK_Thread * t );  // unlocks list
public:
    /** constructor for threads manager.
     * \param ticks_per_second the number of ticks per second that
     *       the timer manager should provide to timer services.
     */
    PK_Threads( int ticks_per_second );
    /** destructor */
    ~PK_Threads( void );
    /** search for a thread by specifying it's name string.
     * \param name the full name of the thread
     * \return a pointer to the PK_Thread base object.
     *          it will have to be upcast to the derived type
     *          if you need to access fields in the derived type.
     */
    PK_Thread * find_name( const char * name );
    /** search for a thread by specifying it's pthread_t id.
     * every thread is, underneath, a pthread.  the pthread_t id  of
     * a thread can be obtained by calling PK_Thread::get_id.
     * \param id  the pthread_t id of the thread
     * \return a pointer to the PK_Thread base object.
     *          it will have to be upcast to the derived type
     *          if you need to access fields in the derived type.
     */
    PK_Thread * find_id( pthread_t id );
    /** the main scheduling loop. 
     * PK_Thread based thread objects may exist but do not run until
     * this method is called. while threads exist, this function does
     * not return. when the last thread exits (returns from it's entry
     * method) then this method will return to the caller.
     */
    void run( void );
};

/** \cond INTERNAL */
extern PK_Threads * th;
enum { PK_THREAD_LIST, PK_THREAD_IDHASH, 
       PK_THREAD_NAMEHASH, PK_THREAD_NUMLISTS };
struct PK_Thread_startup_sync {
    bool             waiting;
    bool             needed;
    bool             runner_active;
    pthread_cond_t   cond;
    pthread_mutex_t  mutex;
};
/** \endcond */

/** base class for a thread.
 * derive your thread class from this one.
 * your derived thread classes should have:
 * <ul>
 * <li> a constructor which calls <ul> <li> \ref set_name and
 *      <li> \ref resume </ul>
 * <li> an implementation of \ref entry
 * </ul> */
class PK_Thread {
    char     * name;
    pthread_t  id;
    PK_Thread_startup_sync * startup;
    static void * _entry( void * _pk_thread );
protected:
    /** virtual destructor, providing hook into
     * your derived object's destructor. */
    virtual ~PK_Thread( void ) { free( name ); }
/**\name PK_Thread Initialization
 * an object derived from this one implements a thread. the derived
 * object must provide the following:
 * <ul>
 * <li> an \ref entry method, which will be the \a main of the thread
 * <li> a constructor which calls
 *    <ul> <li> \ref set_name <li> \ref resume </ul>
 * </ul> */
/**@{*/
    /** entry point to the thread. 
     * your derived class implementing a thread should override this
     * member with your own. this method will then become the "main"
     * for that thread. when your entry method returns, the thread is
     * then considered complete and your derived object will be deleted. */
    virtual void entry( void ) = 0;
    /** make this thread runnable.
     * when a new thread is created, it is created in a suspended state.
     * the \ref entry method is not called immediately. this function
     * makes the thread runnable so the \ref entry method is called (once
     * PK_Threads::run is called, of course). This allows for startup 
     * synchronization between the underlying pthread and the object,
     * as well as between the underlying pthreads and the PK_Threads::run
     * method.
     * \note typically \ref resume should be called from your derived-object
     * constructor, unless your objects have some other need for further
     * start-up synchronization. */
    void resume( void );
    /** set the next name of the thread. 
     * this should be done early in the derived-object constructor, and
     * should be done only once (though it is not required/enforced).
     * \note  if a thread never calls set_name, it will be assigned
     *     a randomly-generated unique name.
     * \param n  a format string for a sprintf-style argument list
     *        for forming the name of the thread. */
    void set_name( const char * n, ... ) {
        char * oldn = name, * newn;
        char tempn[100];
        va_list ap;
        va_start(ap,n);
        vsnprintf(tempn, sizeof(tempn)-1, n, ap);
        tempn[sizeof(tempn)-1] = 0;
        va_end(ap);
        newn = strdup( tempn );
        th->unhash_name( this );
        name = newn;
        th->rehash_name( this );
        free( oldn );
    }
/**@}*/
public:
    /** constructor */
    PK_Thread( void );
    /** \cond INTERNAL */
    friend class PK_Threads;
    LListLinks <PK_Thread> links[PK_THREAD_NUMLISTS];
    /** \endcond */
/**\name PK_Thread Identity */
/**@{*/
    /** return the name string bound to the thread.
     * \return a pointer to the name string */
    const char * get_name( void ) const { return name; }
    /** return the pthread_t id of the underlying thread.
     * \return the pthread_t id of the underlying thread. */
    const pthread_t get_id( void ) const { return id; }
/**@}*/
/**\name PK_Thread Message Queues
messages are derived from \ref pk_msg_int using the macros
 \ref PkMsgIntDef and \ref PkMsgIntDef2. messages are then 
constructed, populated, and enqueued by the sender.  the pointer
passes through the queue, and the recipient then consumes the 
contents and either deletes or sends the message on. a recipient
may listen to multiple queues at once, may change the list to listen
to on the fly, and may select their priorities relative to each other
on the fly. 
\todo implement an object for listing message queues to listen to that
can add and delete queues and change their relative priorities on the fly. */
/**@{*/
    /** create a message queue.
     * \note message classes (derived from pk_msg_int) may be defined using
     *      the \ref PkMsgIntDef and \ref PkMsgIntDef2 macros.
     * \param name   a format string for a sprintf-style argument list.
     *          a unique name may be built for the message queue name,
     *          which can be looked up using PK_Thread::msg_lookup.
     * \return a message queue id which may be used in PK_Thread::msg_send,
     *          PK_Thread::msg_recv, or PK_Thread::msg_destroy. */
    static int msg_create( const char * name, ... ) {
        va_list ap;
        char tempn[100];
        va_start(ap,name);
        vsnprintf(tempn,sizeof(tempn)-1,name,ap);
        tempn[sizeof(tempn)-1] = 0;
        va_end(ap);
        return PK_Messages_global->create( tempn );
    }
    /** search for a message queue by name.
     * \note message classes (derived from pk_msg_int) may be defined using
     *      the \ref PkMsgIntDef and \ref PkMsgIntDef2 macros.
     * \param name  a format string for a sprintf-style argument list.
     *          a unique name may be built for the message queue name.
     * \return <ul> <li> a message queue id which may be used in
     *          PK_Thread::msg_send, PK_Thread::msg_recv, or
     *          PK_Thread::msg_destroy, or <li> NULL if the name is
     *          not found </ul> */
    static int msg_lookup( const char * name, ... ) {
        va_list ap;
        char tempn[100];
        va_start(ap,name);
        vsnprintf(tempn,sizeof(tempn)-1,name,ap);
        tempn[sizeof(tempn)-1] = 0;
        va_end(ap);
        return PK_Messages_global->lookup( tempn );
    }
    /** destroy a message queue.
     * any pending messages on the queue which have not yet been
     * received will be removed from the queue and deleted.
     * \param qid  the message queue id previously returned by
     *             PK_Thread::msg_create (it is presumed that the
     *             creator of the message queue will also be the one
     *             to destroy it, though this is not required/enforced).
     * \return true if the destroy succeeded, false if there was an error.
     *          the only possible error is that the qid does
     *          not currently exist. */
    static bool msg_destroy( int qid ) {
        return PK_Messages_global->destroy( qid );
    }
    /** send a message (enqueue to a message queue).
     * \note message classes (derived from pk_msg_int) may be defined using
     *      the \ref PkMsgIntDef and \ref PkMsgIntDef2 macros.
     * \param qid  the message queue id, previously returned by 
     *          PK_Thread::msg_create or PK_Thread::msg_lookup.
     * \param msg  pointer to a message derived from pk_msg_int base type.
     * \return true if messages was enqueued, false if not. the only
     *         possible failure reason is that the queue id does not
     *         currently exist. */
    static bool msg_send( int qid, pk_msg_int * msg ) {
        return PK_Messages_global->send( qid, msg );
    }
    /** receive a message from a set of message queues.
     * a thread may block waiting for a message on multiple message
     * queues at once. the list of queues to pend on are passed as an
     * array. the order of queue ids in the array represents the
     * relative priority of the queues; i.e. if several messages are
     * waiting in several queues, the first message from the first
     * available queue will be returned. if all specified queues are
     * empty, this method will either return NULL immediately, block
     * for a specified period of time and then either return a message
     * or return NULL when the timeout has expired, or wait forever
     * until a message.
     * \note message classes (derived from pk_msg_int) may be defined using
     *      the \ref PkMsgIntDef and \ref PkMsgIntDef2 macros.
     * \param num_qids  the number of message queue ids in the array
     * \param qids      a pointer to the array of message queue ids
     * \param retqid    if a message is returned, the message queue id
     *                  that the message came from will be written to
     *                  this pointer.
     * \param ticks     the possible values are: <ul>
     *                  <li> -1 : wait forever for a message
     *                  <li> 0 : return NULL immediately
     *                       if no message is available
     *                  <li> >0 : the number of ticks to wait for a message;
     *                       if this time passes without a message, return NULL.
     *                  </ul>
     * \return NULL if no message, or a pointer to a valid message. 
     *         the returned pointer should be checked for message type
     *         and then upcast (using the pk_msg_int::convert method)
     *         to the correct derived type. */
    static pk_msg_int * msg_recv( int num_qids, int * qids,
                                  int * retqid, int ticks ) {
        return PK_Messages_global->recv( num_qids, qids,
                                         retqid, ticks );
    }
/**@}*/
/**\name PK_Thread Semaphore APIs
semaphores can be used for mutual exclusion or for thread-to-thread
signaling. semaphores are handled using \ref PK_Semaphore objects.
the underlying semaphore mechanism is a combination of pthread
mutex and pthread condition. */
/**@{*/
    /** create a semaphore.
     * the semaphore is a counting semaphore which may be used for
     * mutual exclusion or thread-to-thread signalling. see PK_Semaphore
     * for usage.
     * \param name a format string for a sprintf-style argument list.
     *          a unique name may be built for the semaphore name,
     *          which can be looked up using PK_Thread::sem_lookup.
     * \return a PK_Semaphore object pointer; see PK_Semaphore
     *      for usage. */
    static PK_Semaphore * sem_create( const char * name, ... ) {
        va_list ap;
        char tempn[100];
        va_start(ap,name);
        vsnprintf(tempn,sizeof(tempn)-1,name,ap);
        tempn[sizeof(tempn)-1] = 0;
        va_end(ap);
        return PK_Semaphores_global->create( tempn );
    }
    /** search for a semaphore object by name.
     * \param name  a format string for a sprintf-style argument list.
     *          a unique name may be built for the semaphore name.
     * \return <ul> <li> a PK_Semaphore object pointer; see PK_Semaphore
     *         for usage, or <li> NULL if the name is not found </ul> */
    static PK_Semaphore * sem_lookup( const char * name, ... ) {
        va_list ap;
        char tempn[100];
        va_start(ap,name);
        vsnprintf(tempn,sizeof(tempn)-1,name,ap);
        tempn[sizeof(tempn)-1] = 0;
        va_end(ap);
        return PK_Semaphores_global->lookup( tempn );
    }
    /** destroy a semaphore.
     * any waiters blocked on the semaphore will be resumed; behavior
     * after this point, if the pointer is referenced again, is undefined
     * (the memory is freed). it is up to the caller to ensure that the 
     * consumers are properly notified that the semaphore can no longer
     * be used.
     * \param s   the PK_Semaphore object pointer */
    static void sem_destroy( PK_Semaphore * s ) {
        PK_Semaphores_global->destroy( s );
    }
/**@}*/
/**\name PK_Thread Timer APIs
a separate thread marks time at a fixed rate, configurable at \ref
PK_Threads construction time. the methods in this group utilise this
capability to provide time services including timed delays, a clock,
and multiple different types of timers. timers may perform the
following services: <ul> <li> send a message <li> execute a function
<li> signal a pthread condition </ul> timers may also be canceled
before they expire, and any message held by the timer can be returned
to the canceler. */
/**@{*/
    /** create a timer which sends a message upon expiry.
     * a timer is created which is counting at the configured
     * ticks-per-second rate. when the timer expires, the
     * specified message will be enqueued to the specified message
     * queue id.
     * \param ticks  the number of system ticks this timer should live.
     * \param qid    the message queue id where the timer message
     *               should be sent when the timer expires.
     * \param msg    the message that should be sent.
     *           \note the msg should now be considered to be owned by
     *             the timer; the caller should no longer attempt to 
     *             reference this pointer
     * \return a timer identifier value which can be passed to 
     *     \ref timer_cancel if needed. */
    static int timer_create( int ticks, int qid, pk_msg_int * msg ) {
        return PK_Timers_global->create( ticks, qid, msg );
    }
    /** create a timer which calls a function upon expiry.
     * a timer is created which is counting at the configured
     * ticks-per-second rate. when the timer expires, the
     * specified function will be called with the specified argument.
     * \param ticks  the number of system ticks this timer should live.
     * \param func  a pointer to a function which will be called. this
     *           function must take a single void* as an argument.
     *           \note this must not be a normal class member. it must
     *              be either a static class method or a non-object
     *              C-style function.
     * \param arg   an argument to be passed to the function when it is
     *           called. one suggestion is to pass an object pointer,
     *           so that a static class method may access the class 
     *           data members and other class methods.
     * \return a timer identifier value which can be passed to 
     *     \ref timer_cancel if needed.
     *
     * sample code: \code
     * class MyThread : public Thread {
     *    ...;
     *    static void MyTimerFunc(void*arg) {
     *       MyThread * me = (MyThread*) arg;
     *       me->...;
     *    }
     *    void entry( void ) { 
     *       int tid = timer_create( ticks, 
     *                               &MyTimerFunc,
     *                               (void*) this );
     *    }
     * };
     * \endcode */
    static int timer_create( int ticks, void (*func)(void *), void * arg ) {
        return PK_Timers_global->create( ticks, func, arg );
    }
    /** create a timer which pokes a pthread condition upon expiry.
     * a thread may block on a pthread condition. this timer can be set
     * to signal that condition when the timer expires.  the code blocking
     * on the condition may then check the PK_Timeout_Obj::timedout field
     * to see if the condition was signaled by the timer or by another 
     * source.
     * \note the reason why this might be preferable to
     *      pthread_cond_timedwait is that timedwait does not provide
     *      any mechanism to know how much time is left on the timeout.
     *      this mechanism (with \ref timer_cancel) provides that.
     * \param ticks   the duration of the timer in system ticks
     * \param pkto    a pointer to a user-owned PK_Timeout_Obj
     * \return a timer identifier value which can be passed to 
     *     \ref timer_cancel if needed.
     *
     * sample code: \code
     * void MyThread::entry( void ) {
     *    PK_Timeout_Obj  timeoutObject;
     *    timeoutObject.cond = &myCondition;
     *    int tid = timer_create(ticks, &timeoutObject);
     *    while (1) {
     *       pthread_cond_wait(&myCondition,&myMutex);
     *       if (condition i was waiting for)
     *          handle_condition();
     *       else if (timeoutObject.timedout)
     *          handle_timeout();
     *    }
     * }
     * \endcode */
    static int timer_create( int ticks, PK_Timeout_Obj * pkto ) {
        return PK_Timers_global->create( ticks, pkto );
    }
    /** cancel a timer (destroy it before it expires), recover the message
     * it held, and return the time left on the timer.
     * if the timer is a message-delivering timer, it is possible to
     * have that message returned to the caller during cancel. (this
     * would be useful if the timer is a statically-allocated message,
     * rather than a dynamic one (prevents this method from trying to
     * call \a delete on it)). also, if a timer has not yet expired, it
     * is possible to discover how much time is left before it
     * expires.
     * \param tid   the timer id of the timer to cancel
     * \param ptr   a pointer to a pointer which will be filled in with 
     *        a pointer to the message contained in the timer. if the timer
     *        is not a message-sending timer, or the timer has already expired,
     *        this pointer will be filled as NULL.  if this parameter is
     *        passed as NULL, and the timer is a message-sending timer, the
     *        message contained in the timer will be deleted using \a delete.
     * \param remaining  a pointer to an integer which will be filled in
     *        with the number of ticks remaining before this timer was to 
     *        expire.  if this parameter is passed as NULL, it indicates the
     *        caller is not interested in the remaining time.
     * \return <ul> <li> true if the timer was canceled, or <li> false
     *    if the timer id does not exist (the cancel may have been in
     *    a race with timer expiry (and lost)). </ul>
     *
     * sample code: \code
     * pk_msg_int * recoveredMessage;
     * int remainingTime;
     * bool cancelSuccess = timer_cancel( timerId,
     *                                    &recoveredMessage,
     *                                    &remainingTime );
     * // if cancelSuccess is true,
     * //    remainingTime now contains the number of ticks left,
     * //    and recoveredMessage contains a pointer to the message
     * //      (if it was a message-sending timer) or NULL otherwise.
     * \endcode */
    static bool timer_cancel( int tid, pk_msg_int ** ptr, int * remaining ) {
        return PK_Timers_global->cancel( tid, ptr, remaining );
    }
    /** cancel a timer (destroy it before it expires).
     * this is equivalent to timer_cancel(tid,NULL,NULL);
     * \param tid  the timer id returned by \ref timer_create
     * \return <ul> <li> true if the timer was canceled, or <li> false
     *    if the timer id does not exist (the cancel may have been in
     *    a race with timer expiry (and lost)). </ul> */
    static bool timer_cancel( int tid ) {
        return PK_Timers_global->cancel( tid );
    }
    /** return the configured ticks-per-second.
     * the ticks-per-second was specified to PK_Threads::PK_Threads
     * constructor when PK_Threads was initialized.
     * \return the number of ticks each second. */
    static int tps( void ) { return PK_Timers_global->tps(); }
    /** go to sleep (block the thread) for a period of time.
     * the thread will block for the specified number of ticks.
     * \note  this function has the same name as the standard unix
     *     function \a sleep, whose argument is in units of seconds.
     *     this method's units are ticks (see \ref tps).  this confusion
     *     could be considered a bug. 
     * \param ticks   the number of ticks to sleep */
    static void sleep( int ticks ) { return PK_Timers_global->sleep( ticks ); }
    /** get the number of ticks that have happened since initialization.
     * the global time increments at \ref tps. this method returns the current
     * instantaneous value. 
     * \note if the program is up and running long enough, the value returned
     *    will wrap around to zero.  although, a typical value for \ref tps is
     *    10, at which rate, a uint64_t will take a while to wrap, around
     *    584942417 years. */
    static uint64_t get_time( void ) { return PK_Timers_global->get_time(); }
/**@}*/
/**\name PK_Thread File Descriptor APIs 
a mechanism is provided to allow a PK_Thread to manage message queues
and file descriptors at the same time with one message-loop. the following
apis provide a gateway.  there is a thread dedicated to running \a select,
and the following methods communicate with that thread to when a user thread
wishes to monitor a file descriptor.  the user may register a descriptor
for read (to send a message when data is available for read), write (to
send a message when a descriptor has available buffer space for writing),
or both.  the message sent is PK_FD_Activity. */
/**@{*/
    /** register a file descriptor for monitoring activity.
     * if activity is detected, a PK_FD_Activity message is sent to the
     * indicated message queue id, with the indicated obj pointer included.
     * \note if activity is detected and the PK_FD_Activity message
     *       is sent, the descriptor is automatically unregistered
     *       and must be reregistered if monitoring should continue.
     * \param fd   the file descriptor to monitor for activity
     * \param rw   the activity to monitor for
     * \param qid  the message queue to send PK_FD_Activity to if 
     *             activity is detected.
     * \param obj  a user data item that will be included in the
     *             PK_FD_Activity if activity occurs.
     * \return a unique \a pkfdid identifer representing the registration
     *     transaction; this identifier can be specified to
     *     \ref unregister_fd if the registration is no longer required. */
    static int /*pkfdid*/ register_fd( int fd, PK_FD_RW rw,
                                       int qid, void * obj ) {
        return PK_File_Descriptors_global->register_fd( fd, rw, qid, obj ); }
    /** cancel a registration on a file descriptor.
     * the monitoring thread is notified to stop monitoring the
     * descriptor represented by the specified \a pkfdid.
     * \note there is a race condition with the monitoring thread; by
     *       the time this method begins attempting to notify the monitoring
     *       thread, the monitoring thread may have already detected the
     *       registered activity and enqueued the PK_FD_Activity message,
     *       and the calling thread may simply have not yet dequeued it.
     *       in this case, this method will return NULL, and the thread
     *       will receive the \ref PK_FD_Activity message anyway. the
     *       application must be prepared to handle this possibility.
     * \param pkfdid  the unique identifier returned by \ref register_fd.
     * \return the \a obj pointer originally passed to \ref register_fd,
     *      if the \a pkfdid is still valid; if the activity has already 
     *      occurred, the registration will no longer exist, and this
     *      method will return NULL. */
    static void * unregister_fd( int pkfdid ) {
        return PK_File_Descriptors_global->unregister_fd( pkfdid ); }
/**@}*/
};

#endif /* __THREADS_H__ */
