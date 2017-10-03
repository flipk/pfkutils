/*
 * This code is written by Phillip F Knaack. This code is in the
 * public domain. Do anything you want with this code -- compile it,
 * run it, print it out, pass it around, shit on it -- anything you want,
 * except, don't claim you wrote it.  If you modify it, add your name to
 * this comment in the COPYRIGHT file and to this comment in every file
 * you change.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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

ThreadTimers :: ThreadTimers( int _tps, int _hash_size )
{
    tps = _tps;
    hash_size = _hash_size;
    die = false;
    cmds = cmds_end = NULL;
    tick = 0;
    numtimers = 0;

    hash = new timerParams*[ hash_size ];

    int i;
    for ( i = 0; i < hash_size; i++ )
        hash[i] = NULL;
    q = NULL;

    mytid = th->create( "timer", Threads::NUM_PRIOS-1, 16384,
                        false, _thread, (void*)this );
}

ThreadTimers :: ~ThreadTimers( void )
{
    int i;
    timerParams * hp, * nhp;
    for ( i = 0; i < hash_size; i++ )
        for ( hp = hash[i]; hp != NULL; hp = nhp )
        {
            nhp = hp->next;
            delete hp;
        }
    delete[] hash;
}

void
ThreadTimers :: printtimers( FILE * f )
{
#define FORMAT1 "timerid","action","arg","en","onq","tickrel"
#define FORMAT2 "%11s %6s %3s %2s %3s %7s\n"
#define FORMAT3 "%11d %6s %3d %2d %3d %7d\n"
#define FORMAT4 "-------------------------------------\n"

    fprintf( f, FORMAT2, FORMAT1 );
    fprintf( f, FORMAT4 );

    timerParams * p;
    for ( p = q; p != NULL; p = p->next )
        fprintf( f, FORMAT3,
                 p->timerid, 
                 p->a == RESUME_TID ? "resume" : "msg",
                 p->a == RESUME_TID ?
                 p->mtid.tid : p->mtid.m->dest.mid.get(),
                 p->enabled, p->onq, p->tickrel );

#undef  FORMAT1
#undef  FORMAT2
#undef  FORMAT3
}

void
ThreadTimers :: enqueue_cmd( timerCommand * c )
{
    if ( cmds )
    {
        cmds_end->next = c;
        cmds_end = c;
    }
    else
    {
        cmds = cmds_end = c;
    }
    th->resume( mytid );
}

timerCommand *
ThreadTimers :: dequeue_cmd( void )
{
    timerCommand * c = cmds;

    if ( c )
    {
        cmds = c->next;
        if ( cmds == NULL )
            cmds_end = NULL;
    }

    return c;
}

timerParams *
ThreadTimers :: find_timerid( int timerid )
{
    int h = timerid % hash_size;
    timerParams * p;
    for ( p = hash[h]; p != NULL; p = p->next )
        if ( p->timerid == timerid )
            break;
    return p;
}

timerParams *
ThreadTimers :: new_timer( int ticks )
{
    int timerid;
    while ( 1 )
    {
        timerid = random();
        if ( find_timerid( timerid ) == NULL )
            break;
    }
    return new timerParams( timerid, ticks );
}

int
ThreadTimers :: set( int ticks, Message * m )
{
    timerCommand * c = new timerCommand;
    c->a = SET_TIMER;
    c->p = new_timer( ticks );
    c->p->setv( m );
    int timerid = c->p->timerid;
    enqueue_cmd( c );
    return timerid;
}

int
ThreadTimers :: set( int ticks, Threads::tid_t tid )
{
    timerCommand * c = new timerCommand;
    c->a = SET_TIMER;
    c->p = new_timer( ticks );
    c->p->setv( tid );
    int timerid = c->p->timerid;
    enqueue_cmd( c );
    return timerid;
}

bool
ThreadTimers :: cancel( int timerid, Message ** m )
{
    timerParams * p = find_timerid( timerid );
    if ( p == NULL )
        return false;
    if ( m != NULL )
        *m = p->get_m();
    timerCommand * c = new timerCommand;
    c->a = CANCEL_TIMER;
    c->p = p;
    p->enabled = false;
    enqueue_cmd( c );
    return true;
}



// each time sigalrm arrives from the interval timer,
// write one byte thru the pipe to the helper thread.

// static
void
ThreadTimers :: sighand( int s )
{
    char dummy = 0;
    th->timers->writeretval = ::write( th->timers->p[1], &dummy, 1 );
}

// the helper process does nothing but set an interval timer
// for once every 1/tps seconds. this triggers the above signal
// handler, which sends one byte thru the pipe. thus the thread
// marks the passage of time by observing each byte thru the pipe.

void
ThreadTimers :: setup_helper( void )
{
    pipe( p );   // helper writes to 1, thread reads from 0

    if ( th->do_unix_fork() == 0 )
    {
        struct itimerval itv;
        struct sigaction sa;

        itv.it_interval.tv_sec = 0;
        itv.it_interval.tv_usec = 1000000 / tps;
        itv.it_value.tv_sec = 0;
        itv.it_value.tv_usec = 1000000 / tps;

#if defined(SUNOS)
        sa.sa_handler = (void(*)())sighand;
#else
        sa.sa_handler = sighand;
#endif
        sa.sa_flags = 0;
        sigemptyset( &sa.sa_mask );
        sigaddset( &sa.sa_mask, SIGALRM );

        close( p[0] );
        sigaction( SIGALRM, &sa, NULL );
        setitimer( ITIMER_REAL, &itv, NULL );

        writeretval = 1;
        while ( writeretval > 0 )
            ::select( 0, NULL, NULL, NULL, NULL );

        close( p[1] );
        _exit( 0 );
    }

    close( p[1] );
}

// this thread maintains a list of timer ids currently 
// active, and their actions to take when they expire.
// each timer is actually on two lists -- a hash table,
// hashed by timerid for easy search, and a sorted list
// which is sorted by timer expiry order. all manipulations
// to this list are performed by this thread, so if some
// other thread wishes to set/cancel, it resumes this thread
// which updates the list. this thread is also resumed 
// every 1/tps seconds by the data arriving from the helper
// process to indicate the passage of time. the thread 
// uses this time tick to expire timers from the sorted
// list and perform the requested action.

// static
void
ThreadTimers :: _thread( void * arg )
{
    ThreadTimers * tt = (ThreadTimers *)arg;
    tt->thread();
}

void
ThreadTimers :: thread( void )
{
    setup_helper();
    int helper_fd = p[0];

    th->register_fd( helper_fd );
    do {

        char dummy;
        int r = th->read( helper_fd, &dummy, 1 );
        if ( r > 0 )
            process_tick();
        process_cmds();

    } while ( die == false );
    close( p[0] );
}

void
ThreadTimers :: process_cmds( void )
{
    timerCommand * c;
    while (( c = dequeue_cmd()) != NULL )
    {
        if ( c->a == SET_TIMER )
            enqueue_p( c->p );
        else if ( c->a == CANCEL_TIMER )
            dequeue_p( c->p );

        if ( c->a == CANCEL_TIMER && c->p->enabled )
            delete c->p;
        delete c;
    }
}

void
ThreadTimers :: process_tick( void )
{
    tick++;
    if ( q == NULL )
        return;
    q->tickrel--;
    while ( q != NULL && q->tickrel <= 0 )
    {
        timerParams * p = q;
        q = q->next;
        p->onq = false;
        dequeue_p( p );

        // expire p

        Message * m = p->get_m();
        if ( m != NULL )
        {
            if ( th->msgs->send( m, &m->dest ) == false )
                printf( "timerid %d failed to send msg\n",
                        p->timerid );
        }
        else
        {
            // this requeueing mechanism is really
            // gross; should investigate a better way
            // to fix it.

            if (( th->valid_tid( p->mtid.tid )) &&
                ( th->resume( p->mtid.tid ) == false ))
            {
                p->tickrel = p->tickarg;
                enqueue_p( p );
                p = NULL;
            }
        }

        if ( p != NULL )
            delete p;
    }
}

void
ThreadTimers :: enqueue_p( timerParams * n )
{
    int h = n->timerid % hash_size;

    n->hnext = hash[h];
    hash[h] = n;
    numtimers++;

    n->onq = true;

    if ( q == NULL )
    {
        q = n;
        return;
    }

    int ticks = n->tickrel;
    timerParams * p, * op;

    for ( op = NULL, p = q;
          p != NULL && ticks > p->tickrel;
          op = p, p = p->next )
    {
        ticks -= p->tickrel;
    }

    if ( p != NULL )
    {
        p->tickrel -= ticks;
        if ( op == NULL )
            q = n;
        else
            op->next = n;
        n->next = p;
    }
    else
    {
        op->next = n;
        n->next = NULL;
    }

    n->tickrel = ticks;
}

void
ThreadTimers :: dequeue_p( timerParams * n )
{
    int h = n->timerid % hash_size;
    timerParams * p, * op;

    for ( op = NULL, p = hash[h]; p != NULL; op = p, p = p->hnext )
    {
        if ( p == n )
            break;
    }

    if ( p == NULL )
    {
        DEBUG1(( 0, "thmsgs", "dequeue_p : bad things happen!" ));
    }
    else
    {
        if ( op == NULL )
            hash[h] = n->hnext;
        else
            op->hnext = n->hnext;
    }

    numtimers--;

    if ( n->onq == false )
        return;

    for ( op = NULL, p = q; p != NULL; op = p, p = p->next )
    {
        if ( p == n )
            break;
    }

    if ( p == NULL )
    {
        DEBUG1(( 0, "thmsgs", "dequeue_p : bad things happen 2!" ));
        return;
    }

    if ( op == NULL )
    {
        q = p->next;
        if ( q != NULL )
            q->tickrel += p->tickrel;
    }
    else
    {
        op->next = p->next;
        if ( op->next != NULL )
            op->next->tickrel += p->tickrel;
    }

    p->next = NULL;
    p->tickrel = 0;
}
