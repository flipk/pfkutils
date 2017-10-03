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

#include <stdarg.h>

// the really nice part: 
//    users can use this to make new threads.
//    full of useful frontends

// NOTE: when you derive an object from this to create your
//       own thread, your constructor MUST eventually call 'resume'
//       on your own tid at some point. see the DemonstrationThread
//       class defn at the end of this file.

class ThreadShortCuts {
protected:
    typedef Threads::tid_t tid_t;
    enum constants {
        NUM_PRIOS        = Threads::NUM_PRIOS,
        INVALID_TID      = Threads::INVALID_TID,
        WAIT_FOREVER     = Threads::WAIT_FOREVER,
        NO_WAIT          = Threads::NO_WAIT,
        SELECT_FOR_WRITE = Threads::SELECT_FOR_WRITE
    };
    enum fd_mq_t {
        FOR_NOTHING      = ThreadMessages::FOR_NOTHING,
        FOR_READ         = ThreadMessages::FOR_READ,
        FOR_WRITE        = ThreadMessages::FOR_WRITE,
        FOR_ERROR        = ThreadMessages::FOR_ERROR
    };
    static const int default_prio = 10;
    static const int default_stack_size = 16384;
    int tps( void ) { return th->timers->get_tps(); }
    int tick( void ) { return th->timers->get_tick(); }
    int my_eid( void ) { return th->msgs->get_my_eid(); }
    bool sleep( int t ) { return th->sleep( t ); }
    void yield( void ) { th->yield(); }
    bool suspend( tid_t t ) { return th->suspend( t ); }
    bool resume( tid_t t ) { return th->resume( t ); }
    void register_fd( int fd ) { th->register_fd( fd ); }
    int read( int fd, void * buf, int sz ) {
        return th->read( fd, buf, sz );
    }
    int write( int fd, void * buf, int sz ) {
        return th->write( fd, buf, sz );
    }
    int select( int nrfds, int * rfds, int nwfds, int * wfds,
                int nofds, int * ofds, int ticks ) {
        return th->select( nrfds, rfds, nwfds, wfds,
                           nofds, ofds, ticks );
    }
    tid_t create( char * name, int prio, int stack, 
                  bool susp, Threads::FUNCPTR func, void * arg ) {
        return th->create( name, prio, stack, susp, func, arg );
    }
    bool register_mq( int &mqid, char * mqname ) {
        return th->msgs->register_mq( mqid, mqname );
    }
    bool unregister_mq( int mqid ) {
        return th->msgs->unregister_mq( mqid );
    }
    bool lookup_mq( int eid, int &mqid, char * mqname ) {
        return th->msgs->lookup_mq( eid, mqid, mqname );
    }
    bool send( Message * m, MessageAddress * ma ) {
        return th->msgs->send( m, ma );
    }
    Message * recv( int numqs, int * mqs, int * mqout, int timeout ) {
        return th->msgs->recv( numqs, mqs, mqout, timeout );
    }
    bool register_eid( int eid, int mqid ) {
        return th->msgs->register_eid( eid, mqid );
    }
    bool unregister_eid( int eid ) {
        return th->msgs->unregister_eid( eid );
    }
    bool register_fd_mq( int fd, void * arg, fd_mq_t t, int mqid ) {
        return th->msgs->register_fd_mq(
            fd, arg, (ThreadMessages::fd_mq_t)t, mqid );
    }
    bool unregister_fd_mq( int fd ) {
        return th->msgs->unregister_fd_mq( fd );
    }
    int set( int ticks, Message * m ) {
        return th->timers->set( ticks, m );
    }
    int set( int ticks, tid_t tid ) {
        return th->timers->set( ticks, tid );
    }
    bool cancel( int timerid, Message ** m ) {
        return th->timers->cancel( timerid, m );
    }
    bool cancel( int timerid ) {
        return th->timers->cancel( timerid );
    }
    ThreadSemaphore * seminit( char * name, int v ) {
        return th->sems->seminit( name, v );
    }
    void semdelete( ThreadSemaphore * s ) {
        th->sems->semdelete( s );
    }
    int vprintf( char * format, va_list ap ) 
        __attribute__ ((format( printf, 2, 0 )));
    int printf( char * format, ... )
        __attribute__ ((format( printf, 2, 3 )));
    void print_header( bool flag ) {
        th->print_header( flag );
    }
};

class Thread : public ThreadShortCuts {
protected:
    class constructor_failed { };
    tid_t tid;

    Thread( char * name, int prio = default_prio,
            int stack_size = default_stack_size ) {
        if ( th == NULL ) 
        {
            DEBUG1(( 0, "Thread constructor",
                     "thread %s : no global "
                     "th object!\n", name ));
            throw constructor_failed();
        }
        tid = th->create( name, prio, stack_size, true,
                          &_entry, (void*)this );
        if ( tid == INVALID_TID )
        {
            DEBUG1(( 0, "Thread constructor",
                     "error creating thread %s\n", name ));
            throw constructor_failed();
        }
    }
    virtual ~Thread( void ) { }
    virtual void entry( void ) = 0;
private:
    static void _entry( void * arg );
};

// you would create a thread instance of this class
// by calling     'new DemonstrationThread( somevalue )'
// you don't ever have to 'delete' this object, because
// when the thread's main loop exits, the object will
// be automatically deleted.

class DemonstrationThread : public Thread {
    static const int myprio = 10;
    static const int mystack = 16384;
    void entry( void ) { 
        // this is the demo thread main loop
        printf( "the arg passed when this thread was started "
                "is %d\n", demovar );
        printf( "start sleeping for one second\n" );
        sleep( tps() * 1 );
        printf( "done sleeping\n" );
    }
    int demovar;
public:
    DemonstrationThread( int demo_start_condition )
        : Thread( "demothread", myprio, mystack ) {
        printf( "demo thread constructor initializes "
                "things for the thread\n" );
        // when constructor done, allow thread to run
        demovar = demo_start_condition;
        resume( tid );
    }
    ~DemonstrationThread( void ) {
        demovar = 0;
        printf( "demo thread destructor cleans up\n" );
    }
};

//
// sample main loop to create above demo thread:
//
// #include "threads.H"
// int
// main()
// {
//  ThreadParams p;
//  p.my_eid = 1;  // anything but zero
//  Threads th( &p );
//  (void) new DemonstrationThread( random() );
//  th.loop();
//  return 0;
// }
//
