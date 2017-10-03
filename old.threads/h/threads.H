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

#include <setjmp.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include "types.H"
#include "encrypt_iface.H"
#include "magic.H"
#include "../../dll2/dll2.H"

void debug_print( int myerrno, char * hdr, char * format, ... )
    __attribute__ ((format( printf, 3, 4 )));
void _debug_print( int myerrno, char * hdr, char * format, va_list ap );

#define TH_DEBUG(val,x) \
   do { if ( th && (th->debug & ThreadParams::val) ) \
        { debug_print x; }} while ( 0 )
#define TH_DEBUG_ALL(x) \
   do { debug_print x; } while ( 0 )

struct ThreadParams {
    static const int DEBUG_NONE            = 0;
    static const int DEBUG_FD              = 1;
    static const int DEBUG_ENQ             = 2;
    static const int DEBUG_CTXSW           = 4;
    static const int DEBUG_MSGS            = 8;
    static const int DEBUG_MSGSFD          = 16;
    static const int DEBUG_CHECKSTACK      = 32;
    static const int DEBUG_PRINTSTACKINFO  = 64;
    static const int DEBUG_ALL             = 0xffffffff;

    int debug;
    int max_threads;
    int max_fds;
    int max_mqids;
    int max_eids;
    int my_eid;
    int tps;
    int timerhashsize;
    // defaults chosen out of a bithat
    ThreadParams( void ) {
        debug = DEBUG_NONE;
        max_threads = 32;
        max_fds = 32;
        max_mqids = 32;
        max_eids = 4;
        my_eid = 0;  // you must update this field!
        tps = 10;
        timerhashsize = 64;
    }
};

class _Thread;
class ThreadMessages;
class ThreadTimers;
class ThreadSemaphore;
class ThreadSemaphores;
class ThreadPrinter;

class Threads {
public:
    typedef int tid_t;
    typedef void (*FUNCPTR)(void *);
    typedef LList<_Thread,0> _ThreadList;
    enum public_constants {
        SELECT_FOR_READ  = 0x8000,
        SELECT_FOR_WRITE = 0x4000,  // returned by select, too
        FOUND_SEL_READ   = 0x2000,
        FOUND_SEL_WRITE  = 0x1000,
        FOUND_SEL_BADF   = 0x0800,
        INVALID_TID      = -1,
        WAIT_FOREVER     = -1,
        NO_WAIT          = 0,
        NUM_PRIOS        = 32       // 0 lowest 31 highest
    };
    class constructor_failed { };   // exception thrown by Threads()
    Threads( ThreadParams * ) throw ( constructor_failed );
    ~Threads();

    bool init_done;
    int debug;
private:
    int max_fds;
    int max_threads;

    _Thread ** threads;               // all threads, tid is index
    _Thread ** descriptors;           // all fds, fd is index
    UINT16 * descriptor_types;
    _ThreadList readyq [ NUM_PRIOS ]; // linked list array, index is priority
    _Thread * current;                // the thread running now
    _Thread * looper;                 // the 'loop' placeholder
//                                       (how we get out at the exit)
    _Thread * free_later;             // queued memory freeing
    int numthreads;                   // how many threads currently running

    // temp list of threads created between
    // time of constructor and time of looper
    _ThreadList initial_threads;

    // this is how we launch new threads
    jmp_buf launch_jb;

    // file descriptor management
    fd_set th_rfdset;
    fd_set th_wfdset;
    int th_max_fd;

    // additional bytes we alloc for stacks on top of 
    // what user requests
    static const int ADDTL_STACK = 2048;

    // read/write/select use these for flagging who
    // owns what fd when.
    bool valid_fd( int fd );
    bool take_fd( int fd, bool for_read );
    void release_fd( int fd, bool for_read );

    // kill calling thread
    void kill( void );

    // entry points for internal threads
    static void thread_idle( void *arg );

    // find a thread by tid
    _Thread * lookup( tid_t tid ) { 
        if ( tid > max_threads )
            return NULL;
        if ( tid == 0 )
            return current;
        return threads[tid];
    }

    // place threads on readyq or remove them
    bool enqueue( _Thread * );
    void unenqueue( _Thread * );
    _Thread * dequeue( void );

    // find highest priority runnable thread and switch to it
    void reschedule( void );

public:
    // scheduler entry point
    void loop( void );

    // print out list of all threads and their states
    void printinfo( void );
    void printinfo2( FILE * f );

    bool valid_tid( tid_t tid ) {
        if ( threads[tid] != NULL )
            return true;
        return false;
    }

    int get_numthreads( void ) { return numthreads; }

    // suspend a thread
    bool suspend( tid_t tid );

    // resume a thread
    bool resume( tid_t tid );

    // return my threadid
    int tid( void );

    // yield processor to some other thread of equal priority
    void yield( void ) { reschedule(); }

    // thread goes to sleep for some time
    // return true if sleep was completed
    bool sleep( int ticks );

    // perform a fork operation. we need a method
    // because on some platforms (for instance, CYGWIN)
    // fork() must be running on the "main" stack, and
    // can't be done on a thread's stack.
    int do_unix_fork( void );

    // create a new thread
    tid_t create( char * name, int prio, int stacksize,
                  bool suspended, FUNCPTR, void *arg );

    // register a file descriptor so scheduling can be
    // properly done around a thread which blocks on this fd
    void register_fd( int fd );

    // threads versions of read/write/select
    int read( int fd, void * buf, int sz );
    int write( int fd, void * buf, int sz );
    int select( int nrfds, int * rfds,
                int nwfds, int * wfds,
                int nofds, int * ofds, int timeout );

    int printf( char * format, ... )
        __attribute__ ((format( printf, 2, 3 )));
    int vprintf( char * format, va_list ap )
        __attribute__ ((format( printf, 2, 0 )));
    void print_header( bool flag );

    ThreadMessages * msgs;
    ThreadTimers * timers;
    ThreadSemaphores * sems;

private:
    ThreadPrinter * printer;

    // internal threads
    void _thread_idle( void );
    friend class ThreadMessages;
    friend class ThreadSemaphores;

    // used by the do_unix_fork() helper function
    ThreadSemaphore * forksem;
};

extern Threads * th;

#include "threads_messages.H"
#include "threads_timers.H"
#include "threads_semaphores.H"
#include "threads_class.H"
#include "threads_messages_tcp.H"
#include "threads_messages_udp.H"

#endif // __THREADS_H_
