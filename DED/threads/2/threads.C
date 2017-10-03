
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include "threads.H"

#ifndef FD_COPY
#define FD_COPY(from, to) memcpy(to, from, sizeof(*to))
#endif

#ifdef DO_STATIC
#define STATIC static
#else
#define STATIC
#endif

_Thread  * previous_thread;
STATIC Threads     * sig_current_threads;
STATIC _Thread     * sig_current_thread;

#define   blocksig() timers->  blocksig()
#define unblocksig() timers->unblocksig()

static void
ThreadSignalHandler( int s )
{
    sig_current_threads->printinfo();
}

Threads :: Threads( void )
{
    memset( (char*)this, 0, sizeof( *this ));
#if sparc
    // sparc doesn't have siginfo ?
    signal( SIGUSR1, ThreadSignalHandler );
#else
    signal( SIGINFO, ThreadSignalHandler );
#endif
    sig_current_threads = this;
    stdio_sem = seminit( 1 );
    create( &idler, NULL, 8192, 0, "idler" );
    timers = new TimerThread( this, &Threads::timer_tick );
}

Threads :: ~Threads( void )
{
    int i;
    semdelete( stdio_sem );
    // timers deletes itself from thread context
    if ( free_later != NULL )
        free( free_later );
    for ( i = 0; i < MAX_THREADS; i++ )
        if ( threads[i] != NULL )
            delete threads[i];
}

void
Threads :: printinfo( void )
{
    // attempt the semaphore operation.
    // if we can't get it, forget it -- the 
    // user will just have to try again.

    if ( take( stdio_sem, NO_WAIT ) == false )
        return;

    static const char format1[] =
        "%3s %s%-9s %4s %9s %8s %8s\n";
    static const char format2[] =
        "%3d %s%-9s %4d %9s %8d %8d\n";
    fprintf( stderr, "%d threads running:\n", numthreads );
    fprintf( stderr, format1,
             "tid", " ", "name", "prio",
             "state", "stacksz", "margin", "threadstatus" );
    fprintf( stderr, "-----------------------"
             "------------------------\n" );
    for ( int i = 0; i < MAX_THREADS; i++ )
    {
        _Thread * t = threads[i];
        if ( t == NULL )
            continue;
        fprintf( stderr, format2,
                 t->tid,
                 (t == current) ? "*" : " ",
                 t->name,
                 t->prio,
                 t->state_name( t->state ),
                 t->stack_size,
                 t->stack_margin() );
    }

    give( stdio_sem );
}

void
Threads :: enqueue( _Thread * t )
{
    if ( t->on_sleepq == true )
        _unsleep( t );
    int prio = t->prio;
    t->state = _Thread::TH_READY;
    t->next = NULL;
    priomask |= 1 << prio;
    if ( readyq[prio] == NULL )
    {
        readyq[prio] = t;
        readyq_end[prio] = t;
    }
    else
    {
        readyq_end[prio]->next = t;
        readyq_end[prio] = t;
    }
}

_Thread *
Threads :: dequeue( void )
{
    _Thread * ret;
 again:
    UINT32 pm = priomask;
    int prio;
    for ( prio = NUM_PRIOS-1; prio >= 0; prio--, pm <<= 1 )
        if ( pm & 0x80000000 )
            break;
    if ( prio == -1 )
        return NULL;
    ret = readyq[ prio ];
    if ( ret == NULL )
    {
        // error, bit set but nothing in the queue. fix it + try again
        priomask &= ~( 1 << prio );
        goto again;
    }
    readyq[prio] = ret->next;
    if ( ret->next == NULL )
    {
        readyq_end[prio] = NULL;
        priomask &= ~( 1 << prio );
    }
    ret->next = NULL;
    return ret;
}

void
Threads :: remove( _Thread * t )
{
    int prio = t->prio;
    if (( priomask & ( 1 << prio )) == 0 )
        return;
    if ( readyq[ prio ] == t )
    {
        readyq[ prio ] = t->next;
        if ( t->next == NULL )
        {
            readyq_end[ prio ] = NULL;
            priomask &= ~( 1 << prio );
        }
        return;
    }
    _Thread * old, * cur;
    for ( old = NULL, cur = readyq[ prio ];
          cur != NULL && cur != t;
          old = cur, cur = cur->next )
    {
        /* nothing */;
    }
    if ( cur == NULL )
        return;
    old->next = cur->next;
    if ( readyq_end[prio] == cur )
        readyq_end[prio] = old;
    return;
}

int
Threads :: find_max_fd( void )
{
    int i, hm, cur;
    hm = howmany( FD_SETSIZE, NFDBITS );
    for ( i = hm-1; i >= 0; i-- )
    {
        cur = readfds.fds_bits[i] | writefds.fds_bits[i];
        if ( cur != 0 )
            break;
    }
    i++;
    return i * 32;
}

void
Threads :: wakeup_io_threads( int cc, fd_set * r, fd_set * w )
{
    int i, bit, mask, fd, cur, hm;
    _Thread * t;

    hm = howmany( FD_SETSIZE, NFDBITS );
    for ( i = 0, fd = 0; cc > 0; i++, cc-- )
    {
        for (;;)
        {
            cur = r->fds_bits[i] | w->fds_bits[i];
            if ( cur != 0 )
                break;
            if ( ++i == hm )
            {
                return;
            }
            fd += 32;
        }
        for ( mask = 1, bit = 0;
              bit < 32;
              mask <<= 1, bit++ )
        {
            if ( cur & mask )
            {
                t = descriptors[fd + bit];
                if ( t != NULL )
                {
                    if ( t->state != _Thread::TH_READY )
                        enqueue(t);
                    if (t->selectfds &&
                        t->numselectfds < t->maxselectfds)
                    {
                        int ind = t->numselectfds++;
                        t->selectfds[ind] = fd + bit;
                        if ( w->fds_bits[i] & mask )
                        {
                            t->selectfds[ind] |=
                                SELECT_FOR_WRITE;
                        }
                    }
                }
            }
        }
    }
}

void
Threads :: timer_tick( void )
{
    blocksig();
    if ( sleepq != NULL )
    {
        sleepq->ticks--;
        while ( sleepq != NULL && sleepq->ticks <= 0 )
        {
            _Thread * t = sleepq;
            sleepq = t->next;
            t->ticks = 0;
            t->on_sleepq = false;
            enqueue( t );
        }
    }
    unblocksig();
}

bool
Threads :: check_fds( int timeout )
{
    struct timeval to;
    fd_set r, w;
    int cc;

    to.tv_sec = 0;
    to.tv_usec = timeout;

    FD_COPY( &readfds, &r );
    FD_COPY( &writefds, &w );

    cc = ::select( nfds, &r, &w, NULL, &to );
    if ( cc > 0 )
    {
        wakeup_io_threads( cc, &r, &w );
        return true;
    }
    return false;
}

void
Threads :: th_sw( _Thread::threadstate newstate )
{
    blocksig();
    if ( newstate == _Thread::TH_READY )
        enqueue( current );

    current->state = newstate;

    if ( free_later != NULL )
    {
        free( free_later );
        free_later = NULL;
    }

    (void) check_fds( 0 );

    _Thread * old = current;
    current = dequeue();
    current->state = _Thread::TH_CURR;

    if ( old == current )
    {
        unblocksig();
        return;
    }

    // this is tricky. see, we are longjmping to 
    // we-don't-know-where, so leaving sigs blocked
    // is a potentially dangerous thing. but actually
    // we do know where we're longjmping to, because
    // there are only two places that ever setjmp:
    // here, and in Thread::_entry.
    // both of these setjmp places will unblock sigs,
    // so its safe to leave sigs blocked until after
    // the longjmp.

    if (!(_setjmp( old->jb )))
    {
        previous_thread = old;
        sig_current_threads = this;
        sig_current_thread  = current;
        _longjmp( current->jb, 1 );
    }
    unblocksig();
}

void
Threads :: _idler( void )
{
    while ( numthreads > 2 )
    {
        (void) check_fds( 100000 );
        yield();
    }
    timers->die();
}

void
Threads :: loop( void )
{
    if (!_setjmp( exit_thread ))
    {
        current = dequeue();
        if ( current == NULL )
            return;
        current->state = _Thread::TH_CURR;
        _longjmp( current->jb, 1 );
        // NOTREACHED
    }

    if ( free_later != NULL )
    {
        free( free_later );
        free_later = NULL;
    }

    current = NULL;
}

Threads :: tid_t
Threads :: create( _Thread::thread_func func,
                   void * arg, int stacksize,
                   int prio, char * name, bool suspended )
{
    _Thread * n;
    tid_t tid;

    blocksig();

    for ( tid = 0; tid < MAX_THREADS; tid++ )
        if ( threads[tid] == NULL )
            break;

    if ( tid == MAX_THREADS )
    {
        unblocksig();
        return INVALID_TID;
    }

    n = new _Thread( tid, prio, name, stacksize, func, arg, this );

    threads[tid] = n;
    numthreads++;

    if ( suspended )
        n->state = _Thread::TH_SUSPENDED;
    else
        enqueue( n );

    unblocksig();

    if ( current != NULL && current->prio <= n->prio )
        th_sw( _Thread::TH_READY );

    return n->tid;
}

void
Threads :: kill( void )
{
    tid_t tid;
    _Thread * t;

    t = current;
    tid = t->tid;

    // don't let _Thread object free its own stack.
    blocksig();
    if ( free_later != NULL )
        free( free_later );
    free_later = t->stack;
    t->stack = NULL;
    delete t;
    numthreads--;
    threads[tid] = NULL;
    if ( t != current )
    {
        unblocksig();
        return;
    }
    current = dequeue();
    if ( current == NULL )
    {
        unblocksig();
        _longjmp( exit_thread, 1 );
    }
    current->state = _Thread::TH_CURR;
    // no need to unblocksig because code 
    // that we're jumping to does it for us.
    _longjmp( current->jb, 1 );
}

void
Threads :: yield( void )
{
    th_sw( _Thread :: TH_READY );
}

bool
Threads :: suspend( tid_t tid )
{
    if ( tid == 0 )
    {
        th_sw( _Thread::TH_SUSPENDED );
        return true;
    }
    blocksig();
    _Thread * t = threads[ tid ];
    if ( t == NULL )
    {
        unblocksig();
        // error
        return false;
    }
    if ( t->state != _Thread::TH_READY )
    {
        unblocksig();
        // error
        return false;
    }
    remove( t );
    t->state = _Thread::TH_SUSPENDED;
    unblocksig();
    return true;
}

bool
Threads :: resume( tid_t tid )
{
    if ( tid == 0 )
        return false;

    blocksig();
    _Thread * t = threads[ tid ];
    if ( t == NULL )
    {
        unblocksig();
        return false;
    }

    if (( t->state != _Thread::TH_SUSPENDED ) && 
        ( t->state != _Thread::TH_IOWAIT ) && 
        ( t->state != _Thread::TH_SLEEP ) &&
        ( t->state != _Thread::TH_SEMWAIT ))
    {
        unblocksig();
        return false;
    }

    t->state = _Thread::TH_READY;
    enqueue( t );
    // th_sw will unblock sigs
    if ( current != NULL && t->prio >= current->prio )
        th_sw( _Thread::TH_READY );

    return true;
}

void
Threads :: _sleep( int ticks )
{
    _Thread *t, *ot;

    blocksig();
    current->on_sleepq = true;
    if ( sleepq == NULL )
    {
        sleepq = current;
        current->next = NULL;
    }
    else
    {
        for ( ot = NULL, t = sleepq;
              t != NULL && ticks >= t->ticks;
              ot = t, t = t->next )
        {
            ticks -= t->ticks;
        }

        if ( t != NULL )
        {
            t->ticks -= ticks;
            if ( ot == NULL )
                sleepq = current;
            else
                ot->next = current;
            current->next = t;
        }
        else
        {
            ot->next = current;
            current->next = NULL;
        }
    }

    current->ticks = ticks;
    unblocksig();
}

void
Threads :: _unsleep( _Thread * ut )
{
    _Thread *t, *ot;

    if ( ut->on_sleepq == false )
        return;

    ut->on_sleepq = false;

    for ( ot = NULL, t = sleepq;
          t != NULL;
          ot = t, t = t->next )
    {
        if ( t == ut )
            break;
    }

    if ( t == NULL )
    {
        fprintf( stderr, "_unsleep: error\n" );
        return;
    }

    if ( ot == NULL )
    {
        sleepq = t->next;
        if ( sleepq != NULL )
            sleepq->ticks += t->ticks;
    }
    else
    {
        ot->next = t->next;
        if ( ot->next != NULL )
            ot->next->ticks += t->ticks;
    }

    t->next = NULL;
    t->ticks = 0;
}

Threads :: sleep_retval
Threads :: sleep( int ticks )
{
    if ( ticks <= 0 )
        return SLEEP_COMPLETE;

    _sleep( ticks );
    th_sw( _Thread::TH_SLEEP );
    if ( current->ticks > 0 )
        return SLEEP_INTERRUPTED;

    return SLEEP_COMPLETE;
}

int
Threads :: read( int fd, char * buf, int size )
{
    if ( fd < 0 || fd > MAX_FDS )
        return -1;
    if ( descriptors[ fd ] != NULL )
        return -2;
    descriptors[ fd ] = current;
    FD_SET( fd, &readfds );
    nfds = find_max_fd();
    th_sw( _Thread::TH_IOWAIT );
    FD_CLR( fd, &readfds );
    nfds = find_max_fd();
    descriptors[fd] = NULL;
    return ::read( fd, buf, size );
}

int
Threads :: write( int fd, char * buf, int size )
{
    if ( fd < 0 || fd > MAX_FDS )
        return -1;
    if ( descriptors[ fd ] != NULL )
        return -2;
    descriptors[ fd ] = current;
    FD_SET( fd, &writefds );
    nfds = find_max_fd();
    th_sw( _Thread::TH_IOWAIT );
    FD_CLR( fd, &writefds );
    nfds = find_max_fd();
    descriptors[ fd ] = NULL;
    return ::write( fd, buf, size );
}

int
Threads :: select( int nrfds, int * rfds,
                   int nwfds, int * wfds,
                   int nofds, int * ofds,
                   int ticks )
{
    int i, ret;

    if ( ticks == 0 )
    {
        fd_set r, w;
        struct timeval to;
        int cc, nfds, ret, hm, fd, bit;
        long mask, cur;

        hm = howmany( FD_SETSIZE, NFDBITS );

        FD_ZERO( &r );
        FD_ZERO( &w );

        nfds = 0;

        for ( i = 0; i < nrfds; i++ )
        {
            FD_SET( rfds[i], &r );
            if ( rfds[i] >= nfds )
                nfds = rfds[i] + 1;
        }

        for ( i = 0; i < nwfds; i++ )
        {
            FD_SET( wfds[i], &w );
            if ( wfds[i] >= nfds )
                nfds = wfds[i] + 1;
        }

        to.tv_sec = 0;
        to.tv_usec = 0;

        cc = ::select( nfds, &r, &w, NULL, &to );

        ret = 0;

        for ( i = 0, fd = 0; cc > 0; i++, cc-- )
        {
            for (;; fd += 32 )
            {
                cur = r.fds_bits[i] | w.fds_bits[i];
                if ( cur != 0 )
                    break;
                if ( ++i == hm )
                    return ret;
            }
            for ( mask = 1, bit = 0;
                  bit < 32;
                  bit++, mask <<= 1 )
            {
                if ( cur & mask )
                {
                    ofds[ ret++ ] = fd + bit;
                }
            }
        }

        return ret;
    }

#define INSTALL(count, list)                        \
    for (i=0; i < count; i++)                   \
    {                               \
        if (list[i] > MAX_FDS)                  \
        {                           \
            while (i-- > 0)                 \
                descriptors[list[i]] = NULL;        \
            fprintf(stderr,                 \
                "th_select: fd larger than %d was " \
                   "passed in\n", MAX_FDS);         \
            i = 0;                      \
            break;                      \
        }                           \
        descriptors[list[i]] = current;             \
    }

    if (nrfds > 0)
    {
        INSTALL(nrfds, rfds);
        if (i == 0)
            return -1;
    }

    if (nwfds > 0)
    {
        INSTALL(nwfds, wfds);
        if (i == 0)
        {
            while (nrfds-- > 0)
                descriptors[rfds[i]] = NULL;
            return -1;
        }
    }

#undef INSTALL

    blocksig();

    for (i = 0; i < nrfds; i++)
        FD_SET(rfds[i], &readfds);

    for (i = 0; i < nwfds; i++)
        FD_SET(wfds[i], &writefds);

    nfds = find_max_fd();

    current->selectfds = ofds;
    current->numselectfds = 0;
    current->maxselectfds = nofds;

    if (ticks > 0)
    {
        _sleep(ticks);
    } 

    th_sw( _Thread::TH_IOWAIT );
    blocksig();

    current->selectfds = NULL;
    ret = current->numselectfds;
    current->numselectfds = 0;
    current->maxselectfds = 0;

    for (i = 0; i < nrfds; i++)
    {
        FD_CLR(rfds[i], &readfds);
        descriptors[rfds[i]] = NULL;
    }

    for (i = 0; i < nwfds; i++)
    {
        FD_CLR(wfds[i], &writefds);
        descriptors[wfds[i]] = NULL;
    }

    nfds = find_max_fd();
    unblocksig();

    return ret;
}

ThreadSemaphore *
Threads :: seminit( int v )
{
    ThreadSemaphore * ret;

    ret = new ThreadSemaphore( v );
    return ret;
}

void
Threads :: semdelete( ThreadSemaphore * s )
{
    _Thread *t, *nt;

    for ( t = s->waitlist; t; t = nt )
    {
        nt = t->next;
        enqueue( t );
    }

    if ( current != NULL )
        sleep( 1 );

    delete s;
}

bool
Threads :: take( ThreadSemaphore * s, int ticks )
{
    bool retval;

    blocksig();
    
    if ( s->value > 0 )
    {
        s->value--;
        retval = true;
#ifdef SEM_STATS
        s->locks++;
#endif
        goto out;
    }

#ifdef SEM_STATS
    s->contentions++;
#endif

    if ( ticks == NO_WAIT )
    {
        retval = false;
        goto out;
    }

    if ( ticks != WAIT_FOREVER )
        _sleep( ticks );
    else
        current->ticks = 0;

    current->semnext = NULL;
    if ( s->waitlist == NULL )
    {
        s->waitlist = current;
        s->waitlist_end = current;
    }
    else
    {
        s->waitlist_end->semnext = current;
        s->waitlist_end = current;
    }

    th_sw( _Thread::TH_SEMWAIT );
    blocksig();

    if ( s->waitlist == current && s->value > 0 )
    {
        s->value--;
        retval = true;
#ifdef SEM_STATS
        s->locks++;
#endif
        // pulling current out of s->waitlist
        // is easy when we know we're first on the list.
        s->waitlist = current->semnext;
        if ( s->waitlist == NULL )
        {
            s->waitlist_end = NULL;
        }
        goto out;
    }
    else
    {
        retval = false;
        // pull current out of s->waitlist
        _Thread *t, *ot;
        for ( ot = NULL, t = s->waitlist;
              t != current;
              ot = t, t = t->semnext );
        if ( ot == NULL )
        {
            s->waitlist = current->semnext;
            if ( s->waitlist == NULL )
                s->waitlist_end = NULL;
        }
        else
        {
            ot->semnext = current->semnext;
        }
        if ( s->waitlist_end == current )
        {
            s->waitlist_end = ot;
        }
    }
 out:
    unblocksig();
    return retval;
}

bool
Threads :: give( ThreadSemaphore * s )
{
    blocksig();
    s->value++;
    _Thread * t = s->waitlist;
    unblocksig();

    if ( t != NULL )
        resume( t->tid );

    return true;
}

#undef   blocksig
#undef unblocksig

Thread :: Thread( Threads * _th, char * name,
                  int prio = default_prio,
                  int stack_size = default_stack_size )
{
    th = _th;
    tid = th->create( &_entry, (void*) this,
                      stack_size, prio, name, true );
}


void
Thread :: _entry( Threads * _th, void * arg )
{
    Thread * t = (Thread *)arg;
    t->th = _th;
    t->entry();
    delete t;
}

int
Thread :: tps( void )
{
    return th->timers->tps;
}

//
// timer code
//

STATIC TimerThread * local_timer_thread;


TimerThread :: TimerThread( Threads * _th, void (Threads::*_tickfunc)( void ))
    : Thread( _th, "Timer", Threads::NUM_PRIOS-1 )
{
    doexit          = false;
    suspended       = false;
    local_timer_thread  = this;
    sigs            = 0;
    sigsafe         = true;
    tickfunc        = _tickfunc;

    // set up sig handler but don't 
    // start itimer, in case there is
    // a delay from the time this thread
    // is created and threads::loop is called
    // to start threading.

    struct sigaction sa;
    sa.sa_handler = &sighandler;
    sigemptyset( &sa.sa_mask );
    sa.sa_flags = 0;
    sigaction( SIGALRM, &sa, &osa );
    ticks = 0;
#if TIMER_STATS
    sigs_preempted = 0;
#endif
    resume( tid );
}

TimerThread :: ~TimerThread( void )
{
    // shut down sig handler.
    sigaction( SIGALRM, &osa, NULL );
#ifdef TIMER_STATS
    printf( "timer: %d ticks %d preemptions\n", ticks, sigs_preempted );
#endif
}

void
TimerThread :: sighandler( int s )
{
    if ( local_timer_thread == NULL )
        return;

    local_timer_thread->sigs++;
    if ( local_timer_thread->sigsafe &&
         local_timer_thread->suspended == true )
    {
        local_timer_thread->th->resume( local_timer_thread->tid );
    }
#ifdef TIMER_STATS
    else
        local_timer_thread->sigs_preempted++;
#endif
}

void
TimerThread :: die( void )
{
    doexit = true;
    if ( suspended == true )
        resume( tid );
}

void
TimerThread :: entry( void )
{
    struct itimerval itv, oitv;
    itv.it_interval.tv_sec   = 0;
    itv.it_interval.tv_usec  = 1000000 / tps;
    itv.it_value.tv_sec  = itv.it_interval.tv_sec;
    itv.it_value.tv_usec     = itv.it_interval.tv_usec;
    setitimer( ITIMER_REAL, &itv, &oitv );

    while ( doexit == false )
    {
        if ( sigs == 0 )
        {
            suspended = true;
            suspend( 0 );
            suspended = false;
            continue;
        }
        sigs--;
        ticks++;
        if ( tickfunc != NULL )
            (th->*tickfunc)();
    }

    // shut down itimer so that sigalrms don't
    // screw up the process later. perhaps the user
    // wants to keep running in un-threaded mode.

    setitimer( ITIMER_REAL, &oitv, NULL );
}


// ick. watch carefully. 
// this is the sequence:
//   _Thread constructor sets up all object variables.
//   constructor calls platform_setup_thread (abbreviated p_s_t)
//   p_s_t initializes a jmp_buf
//   p_s_t just happens to know which bits of the jmp_buf
//   contain the program counter and stack pointer
//   so p_s_t pokes in the new stack and function pointer.
//   p_s_t also initializes another jb, setup_jb
//   p_s_t also sets a global to point to the _Thread object --
//   we have to do this because there's no (easy) way to
//   get the object context into the thread.
//   p_s_t longjmps to the modified jb, which is pointing
//   to _Thread::thread_entry
//   thread_entry copies the global into a local variable
//   thread_entry sets up the thread's jb
//   thread_entry then longjmps back to p_s_t
//   p_s_t is done so it returns to _Thread constructor.
//   
//   now, later, the scheduler goes to switch to this task
//   for the first time.
//   Threads::th_sw longjmps to the thread's jb which pops
//   us back into thread_entry.
//   thread_entry has the local variable pointing to its object
//   so it uses that to call the entry function for the thread.
//   if/when the entry function for the thread returns, thread_entry
//   will then kill the thread. it must do this because we used
//   a modified jb to enter this thread, and as such there is no
//   way to return from thread_entry (the stack frame is invalid
//   above thread_entry's stack frame).

static _Thread * thread_starting;
static jmp_buf setup_jb;

// static so that we can take a function pointer easily.
void
_Thread :: thread_entry( void )
{
    _Thread * t = thread_starting;
    if ( _setjmp( t->jb ) == 0 )
        longjmp( setup_jb, 1 );
    // the longjmp to here is from th_sw, 
    // which has sigs blocked. must unblock sigs.
    // see comment at end of th_sw.
    t->parent->timers->unblocksig();
    t->func( t->parent, t->arg );
    t->parent->kill();
}


//
// platforms-specific setup for jmp buffers.
//


void
_Thread :: platform_setup_thread( void )
{
    jmp_buf th_entry;
    _setjmp( th_entry );

#if defined(__FreeBSD__) && defined(i386)
    th_entry->_jb[0] = (int)&thread_entry;
    th_entry->_jb[2] = (int)stack + stack_size - 8;
#elif defined(sparc) && defined(__solaris__) // ??
    th_entry[2] = (int)&thread_entry;
    th_entry[1] = (int)stack + stack_size - 64;
    th_entry[3] = (int) ??? we need frame pointer!;
#elif defined(__OpenBSD__) 
    th_entry[0] = (long)&thread_entry;
    th_entry[2] = (long)stack + stack_size - 8;
#else
#error Need setjmp modifications for this platform.
#endif

    if ( _setjmp( setup_jb ) == 0 )
    {
        thread_starting = this;
        longjmp( th_entry, 1 );
        // notreached
    }
    // done
}

int
_Thread :: stack_margin( void )
{
    // count the number of 0xee's still left in 
    // the stack space. this may actually be platform
    // dependent, if a given platform does stack in the
    // reverse order. for now lets assume this works on all.

    UCHAR * bp = stack;
    int count;

    for ( count = 0; count < stack_size; count++ )
        if ( *bp++ != 0xee )
            break;

    return count;
}
