
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

#ifndef __THREADS_H_
#define __THREADS_H_

#include <stdlib.h>
#include <unistd.h>
#include <setjmp.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>

#undef   SEM_STATS
#define TIMER_STATS

#define UCHAR  unsigned char
#define UINT32 unsigned int

#ifdef THREADS_DEBUG
#define VIRTUAL virtual
#else
#define VIRTUAL 
#endif

// forward
class Threads;
class TimerThread;

extern TimerThread * local_timer_thread;
extern Threads * global_th;

// internal representation of thread information.
// user never gets to see a pointer to one of these.

class _Thread {
    enum threadstate {
        TH_NONEXISTANT = 0,
        TH_READY       = 1,
        TH_SUSPENDED   = 2,
        TH_CURR        = 3,
        TH_SLEEP       = 4,
        TH_TSLEEP      = 5,
        TH_IOWAIT      = 6,
        TH_SEMWAIT     = 7,
        TH_MAXSTATENO  = 7     // same as state # above
    };
    static const char * state_name( threadstate s );
    typedef void (*thread_func)( void * arg );
    typedef int tid_t;

    tid_t tid;
    int prio;
    bool on_sleepq;
    int ticks;
    int * selectfds;
    int numselectfds;
    int maxselectfds;
    char * name;
    int errno_save;
    jmp_buf jb;
    UCHAR * stack;
    int stack_size;
    threadstate state;
    thread_func func;
    void * arg;
    Threads * parent;
    class _Thread * next;
    class _Thread * semnext;
    int semretval;
    int stack_margin( void );
    void platform_setup_thread( void );
    static void thread_entry( void );
    friend class Threads;
public:
    _Thread( tid_t _tid, int _prio, char * _name,
             int _stack_size, thread_func _func,
             void * _arg, Threads * _parent )
        : tid( _tid ), prio( _prio ), on_sleepq( false ),
          ticks( 0 ), selectfds( NULL ), numselectfds( 0 ),
          maxselectfds( 0 ), stack_size( _stack_size ),
          state( TH_READY ), func( _func ),
          arg( _arg ), parent( _parent )
        {
            // the _Thread constructor must be called with
            // sigs blocked.

            stack = new UCHAR[ _stack_size ];
//          fprintf( stderr, "stack %#x - %#x\n",
//               stack, stack + _stack_size );
            memset( stack, 0xee, _stack_size );
            name = strdup( _name );
            platform_setup_thread();
        }
    ~_Thread( void ) {
        // if Threads object determines that the current
        // thread is trying to shut itself down, then we
        // can't delete the stack here (we're still running on it)
        // so Threads will clear out stack ptr and free it later.
        if ( stack != NULL )
            delete stack;
        free( name );
    }
    char * get_name( void ) { return name; }
    tid_t  get_tid( void ) { return tid; }
};

class ThreadSemaphore {
public:
    char * get_name( void ) { return name; }
private:
    // this entire class is private, because the
    // user is not allowed to construct, destruct,
    // or access internals of this class. Only 
    // friend Threads is allowed, because someday
    // threads may need to register/deregister semaphores
    // as they are created and destroyed, and this forces
    // the user to always use Threads::seminit and Threads::semdelete
    // to create/destroy semaphores.

    char * name;
    int value;
    _Thread * waitlist;
    _Thread * waitlist_end;
    ThreadSemaphore( int v, char * _name ) {
#ifdef SEM_STATS
        locks = contentions = 0;
#endif
        waitlist = waitlist_end = NULL;
        value = v;
        name = _name;
    }
#ifdef SEM_STATS
    ~ThreadSemaphore( void ) {
        printf( "semaphore '%s': %d locks %d contentions\n",
                name, locks, contentions );
    }
    int locks;
    int contentions;
#endif
    friend class Threads;
};


// the thread engine. create one of these and make all your threads
// using the create method. pointers to this one object get passed 
// into all threads.

class Threads {
public:
    typedef _Thread::tid_t tid_t;
    enum constants {
        INVALID_TID  = -1,       // create failure return code
        WAIT_FOREVER     = -1,       // select tick arg
        NO_WAIT      = 0,        //    ditto
        NUM_PRIOS    = 32,
        SELECT_FOR_WRITE = 0x1000,
        MAX_FDS      = 512,
        MAX_THREADS  = 512
    };
    class constructor_failed { };
protected:
    VIRTUAL int do_select( int nfds,
                           fd_set *rfds, fd_set *wfds, fd_set *efds,
                           struct timeval *timeout );
    _Thread * descriptors   [ MAX_FDS ];
    _Thread * threads   [ MAX_THREADS ];
    _Thread * readyq    [ NUM_PRIOS   ];
    _Thread * readyq_end    [ NUM_PRIOS   ];
    _Thread * sleepq;
    _Thread * current;
private:
    int numthreads;
    int nfds;
    fd_set readfds;
    fd_set writefds;
    UINT32 priomask;
    jmp_buf exit_thread;
    void * free_later;
    void enqueue( _Thread * );
    _Thread * dequeue( void );
    void remove( _Thread * );
    int find_max_fd( void );
    void wakeup_io_threads( int cc, fd_set * r, fd_set * w );
    void _idler( void );
    void timer_tick( void );
    bool check_fds( int timeout );
    void _sleep( int ticks );
    void _unsleep( _Thread * t );
    enum th_sw_from {
        TH_SW_CREATE  = 0,
        TH_SW_YIELD   = 1,
        TH_SW_SUSPEND = 2,
        TH_SW_RESUME  = 3,
        TH_SW_SLEEP   = 4,
        TH_SW_READ    = 5,
        TH_SW_WRITE   = 6,
        TH_SW_SELECT  = 7,
        TH_SW_TAKE    = 8
    };
    static const char * const th_sw_from_strings[];
    void th_sw( th_sw_from, _Thread::threadstate newstate );
    static void idler( void * arg ) {
        global_th->_idler();
    }
    void dump_sleepq( char * s ) {
        printf( "sleepq %s: ", s );
        for ( _Thread * t = sleepq; t; t = t->next )
            printf( "%d(%d) ", t->tid, t->ticks );
        printf( "\n" );
    }
    ThreadSemaphore * stdio_sem;
    ThreadSemaphore * malloc_sem;
    static void ThreadSignalHandler( int s );
    void printinfo( void );
    void kill( void );
    friend class _Thread;
public:
    Threads( void );
    VIRTUAL ~Threads( void );
    VIRTUAL void loop( void );

    VIRTUAL tid_t create( _Thread::thread_func, void * arg,
                          int stacksize, int prio, char * name,
                          bool suspended = false );

    // lock a semaphore dedicated for use with stdio
    // operations like printf. the system's 'printf' function
    // is usually not reentrant.

    VIRTUAL void   stdio( void ) { take( stdio_sem, WAIT_FOREVER ); }
    VIRTUAL void unstdio( void ) { give( stdio_sem ); }

    VIRTUAL void lock_malloc( void ) { take( malloc_sem, WAIT_FOREVER ); }
    VIRTUAL void unlock_malloc( void ) { give( malloc_sem ); }

    bool is_running( void ) { return ( current == NULL ) ? false : true; }

    enum sleep_retval { SLEEP_COMPLETE, SLEEP_INTERRUPTED };
    VIRTUAL sleep_retval sleep( int ticks );
    VIRTUAL void yield( void );
    tid_t tid( void ) { return current->tid; }
    char * name() { return current->name; }

    // false means failure
    VIRTUAL bool suspend( tid_t );
    VIRTUAL bool resume( tid_t );

    VIRTUAL int read( int fd, char * buf, int size );
    VIRTUAL int write( int fd, char * buf, int size );
    VIRTUAL int select( int nrfds, int * rfds,
                        int nwfds, int * wfds,
                        int nofds, int * ofds, int ticks );

    // This function is called by the SIGINFO handler.
    // NOTE: this function is called with stdio locked!
    //   if you attempt to lock stdio from this function
    //   you will get a bus fault in th_sw.
    // Also note that if stdio is already locked at the time
    // that SIGINFO arrives, Threads does NOT print the thread
    // info and also does NOT call this function.

    void (*status_function)( void );

    // semaphores

    VIRTUAL ThreadSemaphore * seminit( int v, char * name );
    VIRTUAL void semdelete( ThreadSemaphore * );
    VIRTUAL bool take( ThreadSemaphore *, int ticks );
    VIRTUAL bool give( ThreadSemaphore * );

    // timers

    TimerThread * timers;
};

// to write your own process, derive a class from this one, 
// and implement the virtual 'entry' function. if you want
// entry point arguments, use constructor, and class variables
// accessible to the entry function.

// NOTE: when your constructor for your thread is complete the
// thread is left in the SUSPEND state. if you want your thread
// to run you must th->resume( ) it at some point. one possible 
// place is the last line of the constructor.

class Thread {
protected:
    class constructor_failed { };
    enum constants {
        INVALID_TID  = Threads::INVALID_TID,
        WAIT_FOREVER     = Threads::WAIT_FOREVER,
        NO_WAIT      = Threads::NO_WAIT,
        SELECT_FOR_WRITE = Threads::SELECT_FOR_WRITE,
        MAX_FDS      = Threads::MAX_FDS,
        MAX_THREADS  = Threads::MAX_THREADS
    };
    Threads * th;
    virtual void entry( void ) = 0;
    static const int default_prio = 10;
    static const int default_stack_size = 16384;
    // shorthand functions
    int tps( void );
    int get_ticks( void );
    ThreadSemaphore * seminit( int v, char * name ) {
        return th->seminit( v, name );
    }
    void semdelete( ThreadSemaphore * s ) { th->semdelete( s ); }
    bool take( ThreadSemaphore *s, int ticks ) {
        return th->take( s, ticks );
    }
    bool give( ThreadSemaphore *s ) {
        return th->give( s );
    }
    void stdio( void ) { th->stdio(); }
    void unstdio( void ) { th->unstdio(); }
    Threads::sleep_retval sleep( int ticks ) {
        return th->sleep( ticks );
    }
    void yield( void ) {
        th->yield();
    }
    typedef _Thread::tid_t tid_t;
    bool suspend( tid_t t ) {
        return th->suspend( t );
    }
    bool resume( tid_t t ) {
        return th->resume( t );
    }
    int read( int fd, char * buf, int sz ) {
        return th->read( fd, buf, sz );
    }
    int write( int fd, char * buf, int sz ) {
        return th->write( fd, buf, sz );
    }
    int select( int nrfds, int * rfds,
                int nwfds, int * wfds,
                int nofds, int * ofds, int ticks ) {
        return th->select( nrfds, rfds, nwfds, wfds, 
                           nofds, ofds, ticks );
    }
    Thread( char * name,
            int prio = default_prio,
            int stack_size = default_stack_size );
    virtual ~Thread( void ) { }
    tid_t tid;
private:
    static void _entry( void * arg );
};


// an example of a thread derived from the above class.
// this thread is implemented in threads.C. 

class TimerThread : public Thread {
private:
    void entry( void );
    static void sighandler( int );
    bool doexit;
    bool suspended;
    bool sigsafe;
    int sigs;
#ifdef TIMER_STATS
    int sigs_preempted;
    int sigs_preempted_and_recovered;
#endif
    int ticks;
    void (Threads::*tickfunc)( void );
    struct sigaction osa;
    void blocksig( void ) {
        if ( this != NULL )
            sigsafe = false;
    }
    void unblocksig( void ) {
        if ( this != NULL )
        {
            sigsafe = true;
        }
    }
    void unblocksig_allowtimer( void ) {
        if ( this != NULL )
        {
            sigsafe = true;
            if ( sigs > 1 )
            {
                sigs_preempted_and_recovered++;
                resume( tid );
            }
        }
    }
    friend class Threads;
    friend class _Thread;
public:
    TimerThread( void (Threads::*_tickfunc)( void ));
    ~TimerThread( void );

    void die( void );
    static const int tps = 20;
    int get_ticks( void ) {
        return ticks;
    }
};

#undef VIRTUAL

#if sparc
#define _setjmp setjmp
#define _longjmp longjmp
#endif

#endif /* __THREADS_H_ */
