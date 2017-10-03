
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <signal.h>

#include "threads.H"
#include "threads_internal.H"
#include "threads_messages_internal.H"
#include "threads_timers_internal.H"

struct semWaiter {
    semWaiter * next;
    semWaiter * prev;
    Threads::tid_t tid;
    bool onq;
    bool giver_awoke;
    semWaiter( Threads::tid_t _tid ) {
        tid = _tid;
        next = prev = NULL;
        onq = false;
        giver_awoke = false;
    }
};

void
ThreadSemaphore :: enqueue( semWaiter * w )
{
    w->next = NULL;

    if ( waitlist_end != NULL )
    {
        waitlist_end->next = w;
        w->prev = waitlist_end;
        waitlist_end = w;
    }
    else
    {
        w->prev = NULL;
        waitlist = waitlist_end = w;
    }

    w->onq = true;
}

void
ThreadSemaphore :: unenqueue( semWaiter * w )
{
    if ( w->onq == false )
        return;

    if ( w->next != NULL )
        w->next->prev = w->prev;
    else
        waitlist_end = w->prev;

    if ( w->prev != NULL )
        w->prev->next = w->next;
    else
        waitlist = w->next;

    w->onq = false;
}

semWaiter *
ThreadSemaphore :: dequeue( void )
{
    semWaiter * ret;

    ret = waitlist;
    if ( ret != NULL )
        unenqueue( ret );

    return ret;
}

ThreadSemaphores :: ThreadSemaphores( void )
{
    first = last = NULL;
}

ThreadSemaphores :: ~ThreadSemaphores( void )
{
    ThreadSemaphore * s, * ns;
    for ( s = first; s != NULL; s = ns )
    {
        ns = s->next;
        DEBUG2(( 0, "ThreadSemaphores",
                 "semaphore '%s' not cleaned up at exit",
                 first->name ));
        delete s;
    }
}

ThreadSemaphore *
ThreadSemaphores :: seminit( char * name, int v )
{
    ThreadSemaphore * ret;
    ret = new ThreadSemaphore( name, v );
    ret->next = NULL;
    if ( last != NULL )
    {
        last->next = ret;
        ret->prev = last;
        last = ret;
    }
    else
    {
        ret->prev = NULL;
        first = last = ret;
    }
    return ret;
}

void
ThreadSemaphores :: semdelete( ThreadSemaphore *s )
{
    if ( s->next != NULL )
        s->next->prev = s->prev;
    else
        last = s->prev;

    if ( s->prev != NULL )
        s->prev->next = s->next;
    else
        first = s->next;

    delete s;
}

bool
ThreadSemaphores :: take( ThreadSemaphore *s, int timeout )
{
    if ( s->v > 0 )
    {
        s->v--;
        return true;
    }

    if ( timeout == Threads::NO_WAIT )
        return false;

    semWaiter wait( th->tid() );

    s->enqueue( &wait );
    
    int timerid = -1;
    if ( timeout != Threads::WAIT_FOREVER )
        timerid = th->timers->set( timeout, th->tid() );

    th->current->state = TH_SEMWAIT;
    th->reschedule();

    if ( timeout != Threads::WAIT_FOREVER )
        th->timers->cancel( timerid );

    s->unenqueue( &wait );

    if ( wait.giver_awoke == true && s->v > 0 )
    {
        s->v--;
        return true;
    }

    return false;
}

void
ThreadSemaphores :: give( ThreadSemaphore *s )
{
    s->v++;
    semWaiter * w = s->dequeue();
    if ( w != NULL )
    {
        w->giver_awoke = true;
        th->resume( w->tid );
    }
}

void
ThreadSemaphores :: printsems( FILE * f )
{
    ThreadSemaphore * s;
    fprintf( f, "%-11s %5s\n", "semname", "value" );
    fprintf( f, "-----------------\n" );
    for ( s = first; s; s = s->next )
    {
        fprintf( f, "%-11s %5d\n", s->name, s->v );
    }
}
