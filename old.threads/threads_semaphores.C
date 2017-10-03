
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
    LListLinks<semWaiter> links[1];
    Threads::tid_t tid;
    bool giver_awoke;
    semWaiter( Threads::tid_t _tid ) {
        tid = _tid;
        giver_awoke = false;
    }
};

void
ThreadSemaphore :: enqueue( semWaiter * w )
{
    waitlist.add( w );
}

void
ThreadSemaphore :: unenqueue( semWaiter * w )
{
    if ( !waitlist.onlist( w ))
        return;

    waitlist.remove( w );
}

semWaiter *
ThreadSemaphore :: dequeue( void )
{
    semWaiter * ret;

    ret = waitlist.get_head();
    if ( ret != NULL )
        waitlist.remove( ret );

    return ret;
}

ThreadSemaphores :: ThreadSemaphores( void )
{
}

ThreadSemaphores :: ~ThreadSemaphores( void )
{
    ThreadSemaphore * s, * ns;
    for ( s = sem_list.get_head(); s; s = ns )
    {
        ns = sem_list.get_next(s);
        sem_list.remove(s);
        TH_DEBUG_ALL(( 0, "ThreadSemaphores",
                       "semaphore '%s' not cleaned up at exit",
                       s->name ));
        delete s;
    }
}

ThreadSemaphore *
ThreadSemaphores :: seminit( char * name, int v )
{
    ThreadSemaphore * ret;
    ret = new ThreadSemaphore( name, v );
    sem_list.add( ret );
    return ret;
}

void
ThreadSemaphores :: semdelete( ThreadSemaphore * s )
{
    sem_list.remove( s );
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
    for ( s = sem_list.get_head(); s; s = sem_list.get_next(s) )
    {
        fprintf( f, "%-11s %5d\n", s->name, s->v );
    }
}
