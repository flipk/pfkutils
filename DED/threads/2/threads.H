
#ifndef __THREADS_H_
#define __THREADS_H_

#include <stdlib.h>
#include <unistd.h>
#include <setjmp.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>

#undef SEM_STATS
#undef TIMER_STATS

#define UCHAR unsigned char
#define UINT32 unsigned int

// forward
class Threads;
class TimerThread;

extern TimerThread * local_timer_thread;

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
    static const char * state_name( threadstate s ) {
        static const char * strings[] = {
            "nonexistant",
            "ready",
            "suspended",
            "current",
            "sleep",
            "tsleep",
            "iowait",
            "semwait"
        };
        if ( s > TH_MAXSTATENO )
            return "unknown";
        return strings[s];
    }
    typedef void (*thread_func)( Threads *, void * arg );
    typedef int tid_t;

    tid_t tid;
    int prio;
    bool on_sleepq;
    int ticks;
    int * selectfds;
    int numselectfds;
    int maxselectfds;
    char * name;
    jmp_buf jb;
    UCHAR * stack;
    int stack_size;
    threadstate state;
    thread_func func;
    void * arg;
    Threads * parent;
    class _Thread * next;
    class _Thread * semnext;
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
            stack = new UCHAR[ _stack_size ];
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
};

struct ThreadSemaphore {
    int value;
    _Thread * waitlist;
    _Thread * waitlist_end;
    ThreadSemaphore( int v ) {
#ifdef SEM_STATS
        locks = contentions = 0;
#endif
        waitlist = waitlist_end = NULL;
        value = v;
    }
#ifdef SEM_STATS
    int locks;
    int contentions;
    ~ThreadSemaphore( void ) {
        printf( "semaphore %x: %d locks %d contentions\n",
                this, locks, contentions );
    }
#endif
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
        MAX_FDS      = 64,
        MAX_THREADS  = 64
    };
private:
    _Thread * descriptors[ MAX_FDS ];
    int numthreads;
    int nfds;
    fd_set readfds;
    fd_set writefds;
    UINT32 priomask;
    _Thread * threads    [ MAX_THREADS ];
    _Thread * readyq     [ NUM_PRIOS   ];
    _Thread * readyq_end [ NUM_PRIOS   ];
    _Thread * sleepq;
    _Thread * current;
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
    void th_sw( _Thread::threadstate newstate );
    static void idler( Threads * th, void * arg ) {
        th->_idler();
    }
    void dump_sleepq( char * s ) {
        printf( "sleepq %s: ", s );
        for ( _Thread * t = sleepq; t; t = t->next )
            printf( "%d(%d) ", t->tid, t->ticks );
        printf( "\n" );
    }
    ThreadSemaphore * stdio_sem;
    void kill( void );
    friend class _Thread;
public:
    Threads( void );
    ~Threads( void );
    void loop( void );

    tid_t create( _Thread::thread_func, void * arg,
                  int stacksize, int prio, char * name,
                  bool suspended = false );

    // lock a semaphore dedicated for use with stdio
    // operations like printf. the system's 'printf' function
    // is usually not reentrant.

    void stdio( void ) { take( stdio_sem, WAIT_FOREVER ); }
    void unstdio( void ) { give( stdio_sem ); }

    void printinfo( void );

    enum sleep_retval { SLEEP_COMPLETE, SLEEP_INTERRUPTED };
    sleep_retval sleep( int ticks );
    void yield( void );
    tid_t tid( void ) { return current->tid; }
    // false means failure
    bool suspend( tid_t );
    bool resume( tid_t );

    int read( int fd, char * buf, int size );
    int write( int fd, char * buf, int size );
    int select( int nrfds, int * rfds,
                int nwfds, int * wfds,
                int nofds, int * ofds, int ticks );

    // semaphores

    ThreadSemaphore * seminit( int v );
    void semdelete( ThreadSemaphore * );
    bool take( ThreadSemaphore *, int ticks );
    bool give( ThreadSemaphore * );

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
    Threads * th;
    virtual void entry( void ) = 0;
    static const int default_prio = 10;
    static const int default_stack_size = 16384;
    // shorthand functions
    int tps( void );
    ThreadSemaphore * seminit( int v ) { return th->seminit( v ); }
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

private:
    static void _entry( Threads *, void * arg );
public:
    tid_t tid;

    Thread( Threads * _th, char * name,
            int prio = default_prio,
            int stack_size = default_stack_size );
    virtual ~Thread( void ) { }
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
#if TIMER_STATS
    int sigs_preempted;
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
            sigsafe = true;
    }
    friend class Threads;
    friend class _Thread;
public:
    TimerThread( Threads * _th, void (Threads::*_tickfunc)( void ));
    ~TimerThread( void );

    void die( void );
    static const int tps = 20;
    int get_ticks( void ) {
        return ticks;
    }
};

#if sparc
#define _setjmp setjmp
#define _longjmp longjmp
#endif

#endif /* __THREADS_H_ */
