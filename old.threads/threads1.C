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

//
// non preemptive threads
//
// in this version, ctxsw from thread to thread only occurs
// when a thread does something that blocks, for instance a
// read/write/select (the methods within Threads object, not
// the normal unix ones) or a message queue 'recv', or a 'sleep',
// or a 'yield' or 'suspend(0)'. if a thread doesn't do something
// that causes it to enter the Threads scheduler object, all other
// threads will be starved. (this way you don't have to worry about
// mutual exclusion so much. there are still semaphores though, for
// those applications which require mutual exclusion and may suffer
// context switches in critical regions.)
//
// caveat: threads blocked on file descriptors
// are only resumed when we ctxsw to the idler; and, threads
// working on busy fds only go thru the ctxswitcher when the fd
// stops being active;  thus it can be possible for one thread
// doing lots of fd activity to starve other threads blocked on fds.
//

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
#include "threads_printer_internal.H"

Threads * th = NULL;

Threads :: Threads( ThreadParams * p )
    throw ( constructor_failed )
{
    if ( th != NULL )
    {
        TH_DEBUG_ALL(( 0, "constructor", "th already exists!" ));
        throw constructor_failed();
    }

    init_done = false;

    if ( p->my_eid == 0 || p->my_eid >= p->max_eids )
    {
        ::printf( "Threads::Threads : invalid eid %d!\n"
                  "You must set ThreadParams.my_eid....\n",
                  p->my_eid );
        throw constructor_failed();
    }

    srandom( getpid() * time( NULL ) );

    int i;
    numthreads = 0;
    debug = p->debug;
    max_threads = p->max_threads;
    max_fds = p->max_fds;

    threads          = new _Thread*[ max_threads ];
    descriptors      = new _Thread*[ max_fds ];
    descriptor_types = new UINT16[ max_fds ];

    for ( i = 0; i < max_threads; i++ )
        threads[i] = NULL;

    for ( i = 0; i < max_fds; i++ )
    {
        descriptors[i] = NULL;
        descriptor_types[i] = 0;
    }

    current = NULL;
    looper = NULL;
    free_later = NULL;

    FD_ZERO( &th_rfdset );
    FD_ZERO( &th_wfdset );
    th_max_fd = 0;

    if ( create( "idle", 0, 16384, false,
                 &thread_idle, (void*)this ) == INVALID_TID )
    {
        ::fprintf( stderr,
                   "could not create idler thread!\n" );
        throw constructor_failed();
    }

    // the following code is a critical block; 
    // the global "th"  *must* be set, however construction
    // is not yet complete.  interactions from here to the end
    // of the file with the semaphore'd memory allocator must
    // be done very carefully.

    // one thing that must be done is that we must make sure that
    // the very first 'malloc' or 'new' call is NOT below the 'th'
    // assignment. so to ensure that, allocate some dummy memory
    // before assigning 'th'.

    th = this;
    try {
        msgs = new ThreadMessages( p->max_mqids, p->my_eid,
                                   p->max_eids, p->max_fds );
        sems = new ThreadSemaphores;
        timers = new ThreadTimers( p->tps, p->timerhashsize );
    }
    catch (...) {
        throw constructor_failed();
    }

    printer = new ThreadPrinter;
    forksem = sems->seminit( "fork", 1 );

    signal( SIGPIPE, SIG_IGN );

    init_done = true;
}
