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

// define this only in this file, nowhere else
#define _INCLUDE_THREAD_STATENAMES
#include "threads.H"
#include "threads_internal.H"
#include "threads_messages_internal.H"
#include "threads_timers_internal.H"
#include "threads_printer_internal.H"

#if defined(SUNOS)
static char *
strerror( int errno )
{
    static char retval[32];
    sprintf( retval, "%d", errno );
    return retval;
}
#endif

void
debug_print( int myerrno, char * hdr, char * format, ... )
{
    va_list ap;
    va_start( ap, format );
    _debug_print( myerrno, hdr, format, ap );
    va_end( ap );
}

void
_debug_print( int myerrno, char * hdr, char * format, va_list ap )
{
    char str[ 81 ];
//  struct timeval tv;

//  gettimeofday( &tv, NULL );

// use this code for relative-timestamps
// tho if you're trying to interleave logs from
// two different processes running simultaneously, 
// absolute timestamps work better when sort(1)ing

//  static int start_second = 0;
//  if ( start_second == 0 )
//      start_second = tv.tv_sec;
//  tv.tv_sec -= start_second;

#define PRTIT(stmt)         \
    if ( len > 0 )          \
    {   int ilen;           \
        stmt;               \
        ilen = strlen( pstr ); \
        len -= ilen;        \
        pstr += ilen;   }

    int len = sizeof( str );
    char *pstr = str;

//  PRTIT(( snprintf( pstr, len, "%ld%06ld %d %s : ",
//            tv.tv_sec, tv.tv_usec, getpid(), hdr ) ));
    PRTIT(( vsnprintf( pstr, len, format, ap ) ));
    if ( myerrno != 0 )
        PRTIT(( snprintf( pstr, len, " : %s", 
                          strerror( myerrno )) ));
    PRTIT(( snprintf( pstr, len, "\n" ) ));

#undef  PRTIT

    str[sizeof(str)-2] = '\n';
    str[sizeof(str)-1] = 0;
    if ( th != NULL )
        th->printf( str );
    else
        printf( str );
}

extern void malloclock_die( void );

Threads :: ~Threads( void )
{
    int i;

#if !defined(SOLARIS) && !defined(CYGWIN)
    malloclock_die();
#endif
    th = NULL;
    sems->semdelete( forksem );

    delete timers;
    delete msgs;
    delete sems;

    if ( free_later != NULL )
        delete free_later;

    if ( numthreads > 0 )
    {
        DEBUG2(( 0, "destructor", "not all threads are dead!" ));
        for ( i = 0; i < max_threads; i++ )
        {
            if ( threads[i] != NULL )
            {
                DEBUG2(( 0, "destructor",
                         "Thread %s(%d) still alive",
                         threads[i]->name,
                         threads[i]->tid ));
                delete threads[i];
            }
        }
    }

    delete[] threads;
    delete[] descriptors;
}

bool
Threads :: valid_fd( int fd )
{
    if ( fd > max_fds )
    {
        DEBUG1(( 0, "valid_fd",
                 "fd %d > max_fds (%d)!", fd, max_fds ));
        return false;
    }
    return true;
}

// take ownership of a file descriptor -- if idler selects
// this fd, then it knows from this which thread to resume

bool
Threads :: take_fd( int fd, bool for_read )
{
    if ( !valid_fd( fd ))
        return false;

    if ( descriptors[fd] != NULL )
    {
        DEBUG1(( 0, "take_fd", "fd %d already registered!", fd ));
        return true;
    }

    descriptors[fd] = current;

    if ( for_read )
        FD_SET( fd, &th_rfdset );
    else
        FD_SET( fd, &th_wfdset );

    if ( th_max_fd < fd )
        th_max_fd = fd;

    return true;
}

// release ownership of a file descriptor

void
Threads :: release_fd( int fd, bool for_read )
{
    int i, last;
    if ( !valid_fd( fd ))
        return;

    if ( descriptors[fd] == NULL )
    {
        DEBUG1(( 0, "release-fd", "fd %d already unregistered!", fd ));
        return;
    }

    descriptors[fd] = NULL;

    if ( for_read )
    {
        FD_CLR( fd, &th_rfdset );
    }
    else
    {
        FD_CLR( fd, &th_wfdset );
    }

    for ( last = i = fd; i <= th_max_fd; i++ )
    {
        if ( FD_ISSET( i, &th_rfdset ) ||
             FD_ISSET( i, &th_wfdset ))
        {
            last = i;
        }
    }

    th_max_fd = last;
}

// called when a thread has finished -- can only be called
// from the context of the thread that must die, that is, a
// thread cannot call kill() for another thread. a thread must kill
// itself. this is to ensure cleanup of the thread's resources.

void
Threads :: kill( void )
{
    current->state = TH_DEAD;
    free_later = current;
    numthreads--;
    threads[current->tid] = NULL;
    reschedule();
    // notreached
}

// place thread onto the ready queue in the appropriate
// priority slot. also flag it as ready.

bool
Threads :: enqueue( _Thread * t )
{
    int prio = t->prio;
    if ( readyq[prio].onlist(t) )
    {
        DEBUG0(( 0, "enqueue", "%s already onq", t->name ));
        return false;
    }
    readyq[prio].add( t );
    t->state = TH_READY;
    return true;
}

// remove a thread from the ready queue, wherever it is.
// this is for when the current thread is suspending someone
// other than current who is runnable.

void
Threads :: unenqueue( _Thread * t )
{
    int prio = t->prio;

    if ( !readyq[prio].onthislist(t) )
        return;

    readyq[prio].remove(t);
}

// remove the highest-priority thread from the run queue. 
// called by 'reschedule' to figure out who is supposed to run
// next. the idler thread should *always* be on the run queue, so
// if noone else is runnable we won't return null from this.
// if this ever returns null, that means idler has exited.

_Thread *
Threads :: dequeue( void )
{
    _Thread * ret = NULL;
    int prio;

    for ( prio = NUM_PRIOS-1; prio >= 0; prio-- )
    {
        ret = readyq[prio].get_head();
        if ( ret )
        {
            readyq[prio].remove( ret );
            break;
        }
    }

    return ret;
}

// find the highest-priority runnable 
// thread and context switch to it.

void
Threads :: reschedule( void )
{
    _Thread * from = current;
    _Thread * to;

    // this little bit of hackery forces 'to' to not
    // be a register variable; this eliminates a warning
    // from gcc that 'variable "to" might be clobbered by
    // "longjmp"'
    _Thread ** dummy;
    dummy = &to;

    // if no thread available, choose looper
    // so threads can exit

    if ( from->state == TH_READY )
    {
        // this should never happen; if it does,
        // debug why and fix it.
        DEBUG2(( 0, "reschedule",
                 "WARNING: from->state was not CURR!" ));
        from->state = TH_CURR;
    }

    if ( from->state == TH_CURR )
        enqueue( from );

    to = dequeue();

    if ( to == NULL )
        to = looper;

    to->state = TH_CURR;

    // optimization -- if the thread which is current
    // is the highest priority runnable thread, don't
    // bother with the ctxsw stuff.

    if ( from == to )
        return;

    to->switches++;

    DEBUG0(( 0, "resched", "sw %s to %s", from->name, to->name ));
    // switch to the new thread.

    current = (_Thread *)to;

// NB: this adds considerable expense to context switch time.
//     however, it is immensely valuable in terms of catching bugs.

    if ( to->stacksize && to->stackleft() < 5000 )
    {
        char msg[ 132 ];
        sprintf( msg,
                 "WARNING: STACK OVERFLOW IN TASK %s (%d of %d)\n",
                 to->name, to->stackleft(), to->stacksize );
        ::write( 1, msg, strlen( msg ));
        exit( 1 );
    }

    if (!(_setjmp( from->jb )))
    {
        _longjmp( to->jb, 1 );
    }

    // when a thread exits, its stack can't be freed 
    // immediately, because the thread is still running on it --
    // so it has to be freed up later, such as now, after
    // context-switching to the next thread.

    if ( free_later != NULL )
    {
        delete free_later;
        free_later = NULL;
    }
}

// entry point to threads. this is called to start the scheduler,
// and does not return until the last thread has exited.

void
Threads :: loop( void )
{
    // make a _Thread as a placeholder for the looper.
    // makes it easy to get in and out, just reschedule()
    // to and from it.

    _Thread ex( 0, "looper" );
    ex.tid = -1;
    ex.prio = 0;
    ex.state = TH_SUSPENDED;
    ex.func = NULL;
    ex.arg = NULL;
    looper = &ex;

    // fill in launch_jb 
    // create() uses launch_jb contents as prototype for 
    // new threads. 

    if ( _setjmp( launch_jb ) != 0 )
    {
        // this is how new threads are launched --
        // launch_jb is copied to the _Thread and
        // then the stack-pointer register is modified.
        // so first ctxsw to new thread jumps here
        // and launches entry function for that thread.

        th->current->func( th->current->arg );

        // when the thread's main loop returns (the thread
        // exits) then we drop out here, so we must kill
        // the thread.

        th->kill();
        // notreached
    }

// this is a macro cuz its done in two places.
#define STACK_SETUP_MACRO \
        memcpy( &t->jb, &launch_jb, sizeof( t->jb ));   \
        SETUP_STACK( t->jb, t->stack, t->stacksize )

    // perform last creation steps on the initial threads
    _Thread * t;
    while ( t = initial_threads.get_head() )
    {
        initial_threads.remove(t);
        STACK_SETUP_MACRO;
        if ( t->state == TH_READY )
            enqueue( t );
    }

    // ctxsw to the first thread
    current = looper;
    // we now jump out of looper into user threads.
    // we'll never come back unless all threads exited
    // and the only thing left is looper.
    reschedule();
    // if we get here, it means the last thread is done.
    looper = current = NULL;
}

void
Threads :: printinfo( void )
{
    printinfo2( stdout );
}

void
Threads :: printinfo2( FILE * f )
{
    int i;

#define FORMAT1 "%10s %3d %4d %6d %-8s %8d %8d %8x %8x\n"
#define FORMAT2 "%10s %3s %4s %6s %-8s %8s %8s %8s %8s\n"
#define FORMAT3 "name","tid","prio","swtchs","state","stksz","stkmarg","func","arg"
#define FORMAT4 \
      "-----------------------------------------------------------------------\n"

    fprintf( f, FORMAT2, FORMAT3 );
    fprintf( f, FORMAT4 );

    _Thread * t;

    for ( i = 0; i < max_threads; i++ )
        if (( t = threads[i] ) != NULL )
            fprintf( f, FORMAT1,
                     t->name, t->tid, t->prio, t->switches,
                     t->statename( t->state ) + 3,
                     t->stacksize, t->stackleft(),
                     (int)t->func, (int)t->arg );

#undef  FORMAT1
#undef  FORMAT2
#undef  FORMAT3
#undef  FORMAT4

    fprintf( f, "\n" );
    msgs->printmqs( f );
    fprintf( f, "\n" );

#define FORMAT1 "fd","thread"
#define FORMAT2 "%3s %10s\n"
#define FORMAT3 "%3d %10s\n"
#define FORMAT4 "--------------\n"

    fprintf( f, FORMAT2, FORMAT1 );
    fprintf( f, FORMAT4 );

    for ( i = 0; i < max_fds; i++ )
        if ( descriptors[i] != NULL )
            fprintf( f, FORMAT3, i, descriptors[i]->name );

#undef  FORMAT1
#undef  FORMAT2
#undef  FORMAT3
#undef  FORMAT4

    fprintf( f, "\n" );
    timers->printtimers( f );
    fprintf( f, "\n" );
    sems->printsems( f );
    fprintf( f, "\n" );
    fprintf( f, "%d threads %d tps %d ticks %d timers \n",
             numthreads, timers->get_tps(), timers->get_tick(), 
             timers->get_numtimers() );
}

// suspend the named thread

bool
Threads :: suspend( tid_t tid )
{
    _Thread * t = lookup( tid );

    if ( t == NULL )
        return false;

    unenqueue( t );

    t->state = TH_SUSPENDED;
    reschedule();

    return true;
}

// resume the named thread

bool
Threads :: resume( tid_t tid )
{
    _Thread * t = lookup( tid );

    if ( t == NULL )
        return false;

    // this could get called before loop() is called
    // (during construction of new threads at init time);
    // at that time current will be null

    if ( current != NULL )
    {
        if ( enqueue( t ) == false )
            return false;
        reschedule();
    }
    else
    {
        t->state = TH_READY;
    }

    return true;
}

// return the thread-id. could have been inline
// in the header, except contents of _Thread is private
// to this file.

int
Threads :: tid( void )
{
    return current->tid;
}

// create a new thread.

Threads :: tid_t
Threads :: create( char * name, int prio, int stacksize,
                   bool suspended, FUNCPTR func, void * arg )
{
    _Thread * t;
    int tid;

    stacksize += ADDTL_STACK;

    for ( tid = 0; tid < max_threads; tid++ )
        if ( threads[tid] == NULL )
            break;

    if ( tid == max_threads )
    {
        DEBUG1(( 0, "create", "out of threads" ));
        return INVALID_TID;
    }

    t = new _Thread( stacksize, name );
    threads[tid] = t;

    t->tid   = tid;
    t->prio  = prio;
    t->state = suspended ? TH_SUSPENDED : TH_READY;
    t->func  = func;
    t->arg   = arg;

    if ( current != NULL )
    {
        // platform-dependent. these two steps are also
        // performed in loop() to fire up the initial threads
        STACK_SETUP_MACRO;

        if ( !suspended )
        {
            enqueue( t );
            if ( current != NULL )
                reschedule();
        }
    }
    else
    {
        // threads not running yet, put these tasks on the
        // initial thread list.

        initial_threads.add(t);
    }
    
    numthreads++;

    return tid;
}

// suspend a thread for specified # of ticks
// return true if sleep was completed.
// this thread could be resume'd before sleep is complete.

bool
Threads :: sleep( int ticks )
{
    int timerid = timers->set( ticks, current->tid );
    current->state = TH_SLEEP;
    reschedule();
    if ( timers->cancel( timerid ) == true )
        return false;
    return true;
}

// register a file descriptor. actually this used to do more,
// but all it does now is ensure that operations on the fd
// can't block the scheduler.

void
Threads :: register_fd( int fd )
{
    fcntl( fd, F_SETFL, 
           fcntl( fd, F_GETFL, 0 ) | O_NONBLOCK );
}

// block a thread until data is available 
// for read on a file descriptors.

int
Threads :: read( int fd, void * buf, int sz )
{
    int r;

    r = ::read( fd, buf, sz );
    if ( r >= 0 || errno != EAGAIN )
        return r;

    if ( !take_fd( fd, true ))
    {
        errno = EMFILE;
        return -1;
    }
    current->state = TH_IOWAIT;
    reschedule();
    release_fd( fd, true );

    r = ::read( fd, buf, sz );
    return r;
}

// block a thread until data can be written to a fd

int
Threads :: write( int fd, void * _buf, int sz )
{
    int r, r2 = 0;
    char * buf = (char*) _buf;

    while ( sz > 0 )
    {
        r = ::write( fd, buf, sz );
        if ( r > 0 )
        {
            buf += r;
            sz -= r;
            r2 += r;
            continue;
        }

        if ( r == 0 )
            return r2;

        if ( r < 0 && errno != EAGAIN )
            return r;

        if ( !take_fd( fd, false ))
        {
            errno = EMFILE;
            return -1;
        }
        current->state = TH_IOWAIT;
        reschedule();
        release_fd( fd, false );
    }

    return r2;
}

// block on a set of fds until one or more becomes
// available. also implements a timeout (measured in ticks)

int
Threads :: select( int nrfds, int * rfds,
                   int nwfds, int * wfds,
                   int nofds, int * ofds, int timeout )
{
    int i, r, fd, max = 0;
    fd_set rfdset;
    fd_set wfdset;

    FD_ZERO( &rfdset );
    FD_ZERO( &wfdset );

    // set up all fdsets

    for ( i = 0; i < nrfds; i++ )
    {
        fd = rfds[i];
        if ( timeout != NO_WAIT )
            if ( !take_fd( fd, true ))
            {
                if ( nofds > 0 )
                    *ofds = fd;
                errno = EMFILE;
                return -1;
            }
        FD_SET( fd, &rfdset );
        if ( max < fd )
            max = fd;
    }

    for ( i = 0; i < nwfds; i++ )
    {
        fd = wfds[i];
        if ( timeout != NO_WAIT )
            if ( !take_fd( fd, false ))
            {
                if ( nofds > 0 )
                    *ofds = fd;
                errno = EMFILE;
                return -1;
            }
        FD_SET( fd, &wfdset );
        if ( max < fd )
            max = fd;
    }

    fd_set tmp_wfdset;
    fd_set tmp_rfdset;
    struct timeval tv = { 0, 0 };

    // poll
    tmp_rfdset = rfdset;
    tmp_wfdset = wfdset;
    r = ::select( max+1, &tmp_rfdset, &tmp_wfdset, NULL, &tv );

    // if no fds available immediately, go to sleep
    // until some are.

    if ( r == 0 && timeout != NO_WAIT )
    {
        int timerid = -1;
        if ( timeout != WAIT_FOREVER )
            timerid = timers->set( timeout, current->tid );
        current->state = TH_IOWAIT;
        reschedule();
        if ( timerid != -1 )
            (void) timers->cancel( timerid );

        // poll again
        tmp_rfdset = rfdset;
        tmp_wfdset = wfdset;
        r = ::select( max+1, &tmp_rfdset,
                      &tmp_wfdset, NULL, &tv );
    }

    // must handle EBADF ... see comment in _thread_idle why
    // we must do this in this complex fashion.
    int c = 0;

    if ( r < 0 && errno == EBADF )
    {
        // find bad fd and poke it into 'out' and return
        fd_set fds;
        FD_ZERO( &fds );

        for ( i = 0; i < nrfds; i++ )
        {
            fd = rfds[i];
            FD_SET( fd, &fds );
            if ( ::select( fd+1, &fds, NULL, NULL, &tv ) < 0 )
            {
                ofds[c++] = fd;
                goto release_all;
            }
            FD_CLR( fd, &fds );
        }

        for ( i = 0; i < nwfds; i++ )
        {
            fd = wfds[i];
            FD_SET( fd, &fds );
            if ( ::select( fd+1, NULL, &fds, NULL, &tv ) < 0 )
            {
                ofds[c++] = fd;
                goto release_all;
            }
            FD_CLR( fd, &fds );
        }
    }

 release_all:
    // tear down all fdsets, copyout active fds
    int r2 = r;

    for ( i = 0; i < nrfds; i++ )
    {
        fd = rfds[i];
        if ( r2 > 0 && FD_ISSET( fd, &tmp_rfdset ) && c < nofds )
        {
            ofds[c++] = fd;
            r2--;
        }
        if ( timeout != NO_WAIT )
            release_fd( fd, true );
    }

    for ( i = 0; i < nwfds; i++ )
    {
        fd = wfds[i];
        if ( r2 > 0 && FD_ISSET( fd, &tmp_wfdset ) && c < nofds )
        {
            ofds[c++] = fd | SELECT_FOR_WRITE;
            r2--;
        }
        if ( timeout != NO_WAIT )
            release_fd( fd, false );
    }

    // don't return how many fds select says; 
    // return instead how many fds we actually copied
    // out into 'ofds' array. so if more fds are active
    // than the user specified as the size of 'ofds', we
    // don't confuse the caller by returning a value larger
    // than their argument.

    if ( r > 0 )
        return c;

    return r;
}

int
Threads :: printf( char * format, ... )
{
    va_list ap;
    va_start( ap, format );
    int ret = vprintf( format, ap );
    va_end( ap );
    return ret;
}

int
Threads :: vprintf( char * format, va_list ap )
{
    char printbuf[ PrintMessage::MAX_MSG ], *pb;
    int max = PrintMessage::MAX_MSG - 2;
    int len;
    char * threadname;

    if ( current != NULL )
        threadname = current->name;
    else
        threadname = "[startup]";
    pb = printbuf;

    if ( current )
    {
        if ( current->print_header )
            snprintf( pb, max, "%s(%d): ", threadname, current->tid );
        else
            pb[0] = 0;
    }
    else
        snprintf( pb, max, "%s: ", threadname );

    len = strlen( pb );
    max -= len;
    pb += len;

    vsnprintf( pb, max, format, ap );
    len += strlen( pb );

    printbuf[ PrintMessage::MAX_MSG-1 ] = 0;

    if ( printer->mqid == -1 )
    {
        th->write( 1, printbuf, len );
    }
    else
    {
        PrintMessage * pm = PrintMessage::newPrintMessage(len);
        memcpy( pm->printbody, printbuf, len );
        pm->dest.set( printer->mqid );

        if ( msgs->send( pm, &pm->dest ) == false )
        {
            ::printf( "Threads::printf: could not "
                      "send printmessage!\n" );
            delete pm;
        }
    }

    return len;
}

void
Threads :: print_header( bool flag )
{
    if ( current )
        current->print_header = flag;
}

#if defined( __CYGWIN32__ )
int
Threads :: do_unix_fork( void )
{
    static int forkpid;
    static jmp_buf forkjb1;
    static jmp_buf forkjb2;

    // this is really, and i mean really, gross.
    // can you tell what i'm doing?  and why?

    sems->take( forksem, WAIT_FOREVER );
    switch ( setjmp( forkjb1 ))
    {
    case 0:
        memcpy( &forkjb2, &forkjb1, sizeof( forkjb1 ));
        forkjb2[7] = looper->jb[7];
        longjmp( forkjb2, 1 );
        /* NOTREACHED */
    case 1:
        forkpid = fork();
        longjmp( forkjb1, 2 );
        /* NOTREACHED */
    case 2:
        break;
    }
    sems->give( forksem );

    return forkpid;
}
#else
int
Threads :: do_unix_fork( void )
{
    int ret;
    sems->take( forksem, WAIT_FOREVER );
    ret = fork();
    sems->give( forksem );
    return ret;
}
#endif

// the idler thread

// static
void
Threads :: thread_idle( void * arg )
{
    Threads * th = (Threads *) arg;
    th->_thread_idle();
}

void
Threads :: _thread_idle( void )
{
    // the idler thread runs all the time. it is expected that
    // at any given moment, there is at least one user-thread
    // in existence. since there are five threads total owned
    // by threads library itself (msgsfd, lookupmq, timer, printer
    // and idler) then numthreads must always be greater than
    // five while a user thread still exists. 
    // if the last user thread ever exits, then numthreads drops
    // to five, this while loop drops out, and the idler signals
    // msgs, timer, and printer to exit (msgs object takes the
    // lookupmq and msgsfd tasks with it). then idler exits.
    // once all of these tasks have exited, rescheduler jumps
    // back to looper and Threads.loop() returns to the caller
    // and Threads is dead.

    while ( numthreads > 5 )
    {
        int r, fd;
        fd_set rfdset = th_rfdset;
        fd_set wfdset = th_wfdset;
        struct timeval tv = { 10, 0 };

        // the idler is the only thread for which it is actually
        // ok to block -- block in select until some fd becomes 
        // available. then find the thread that owns that fd and 
        // wake it up. even timer ticks will go thru here, since
        // the timer task has a pipe leading to the timer process.

        r = ::select( th_max_fd+1, &rfdset, &wfdset, NULL, &tv );

        // handle EBADF .... problem with select() is that 
        // you can't easily figure out which fd is the bad one.
        // so to do it you must do a select on each fd one at a
        // time till you find it.  when its found, resume
        // the thread that owns that fd.
        // note that we don't actually have a method to inform
        // that thread directly that the fd is broken. we assume
        // the thread handles read/write errno retvals; also,
        // the select() in this class repeats this same 
        // analysis so it can return to the application. 
        // application must realize that EBADF return from
        // select means that out[0] contains the bad fd.
        // if we wanted to optimize this, we could shortcut
        // Threads::select's work.. but it probably doesn't 
        // save all *that* much effort.
        
        if ( r < 0 && errno == EBADF )
        {
            FD_ZERO( &rfdset );
            FD_ZERO( &wfdset );
            tv.tv_sec = 0;
            for ( fd = 0; fd <= th_max_fd; fd++ )
            {
                if ( !FD_ISSET( fd, &th_rfdset ) &&
                     !FD_ISSET( fd, &th_wfdset ))
                    continue;

                FD_SET( fd, &rfdset );
                FD_SET( fd, &wfdset );
                if ( ::select( th_max_fd+1, &rfdset,
                               &wfdset, NULL, &tv ) < 0 )
                {
                    _Thread * w = descriptors[fd];
                    if ( !readyq[w->prio].onlist(w) )
                        enqueue( w );
                    // resume it now...
                    reschedule();
                }
                FD_CLR( fd, &rfdset );
                FD_CLR( fd, &wfdset );
            }
            continue;
        }

        fd = 0;
        bool changed = false;
        while ( r > 0 && fd <= th_max_fd )
        {
            if ( FD_ISSET( fd, &rfdset ) || 
                 FD_ISSET( fd, &wfdset ))
            {
                _Thread * w = descriptors[fd];
                if ( !readyq[w->prio].onlist(w) )
                    enqueue( w );
                changed = true;
                r--;
            }

            fd++;
        }
        if ( changed )
            reschedule();
    }
    timers->kill();
    msgs->kill();
    printer->kill();
}
