
#ifndef __THREADS_H__
#define __THREADS_H__

#include "dll2.H"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "pk_messages.H"
#include "pk_semaphores.H"
#include "pk_timers.H"
#include "pk_filedescs.H"

class PK_Thread;
class PK_Thread_List;

class PK_Threads {
    PK_Thread_List * thread_list;
    pthread_cond_t  runner_wakeup;
    bool running;
    friend class PK_Thread;
    // following only called by PK_Thread base class
    void add( PK_Thread * t );
    void remove( PK_Thread * t );
    void unhash_name( PK_Thread * t );  // locks list
    void rehash_name( PK_Thread * t );  // unlocks list
public:
    PK_Threads( int _tps );
    ~PK_Threads( void );
    PK_Thread * find_name( char * name );
    PK_Thread * find_id( pthread_t id );
    void run( void );
};

extern PK_Threads * th;

enum { PK_THREAD_LIST, PK_THREAD_IDHASH, 
       PK_THREAD_NAMEHASH, PK_THREAD_NUMLISTS };

class PK_Thread {
    friend class PK_Threads;
    char     * name;
    pthread_t  id;
    struct startup_sync {
        bool             waiting;
        bool             needed;
        bool             runner_active;
        pthread_cond_t   cond;
        pthread_mutex_t  mutex;
    } * startup;
protected:
    static void * _entry( void * _pk_thread );
    virtual void entry( void ) = 0;
    void resume( void );
    // only _entry may delete a PK_Thread
    virtual ~PK_Thread( void ) { free( name ); }
public:
    PK_Thread( void );
    LListLinks <PK_Thread> links[PK_THREAD_NUMLISTS];
    void set_name( char * n, ... ) {
        char * oldn = name, * newn;
        char tempn[100];
        va_list ap;
        va_start(ap,n);
        vsnprintf(tempn, sizeof(tempn)-1, n, ap);
        tempn[sizeof(tempn)-1] = 0;
        va_end(ap);
        newn = strdup( tempn );
        th->unhash_name( this );
        name = newn;
        th->rehash_name( this );
        free( oldn );
    }
    char * get_name( void ) { return name; }
    pthread_t get_id( void ) { return id; }
    // 
    // following are shortcuts for other apis
    //
    // messages - PK_Messages_global
    static int msg_create( char * name, ... ) {
        va_list ap;
        char tempn[100];
        va_start(ap,name);
        vsnprintf(tempn,sizeof(tempn)-1,name,ap);
        tempn[sizeof(tempn)-1] = 0;
        va_end(ap);
        return PK_Messages_global->create( tempn );
    }
    static int msg_lookup( char * name, ... ) {
        va_list ap;
        char tempn[100];
        va_start(ap,name);
        vsnprintf(tempn,sizeof(tempn)-1,name,ap);
        tempn[sizeof(tempn)-1] = 0;
        va_end(ap);
        return PK_Messages_global->lookup( tempn );
    }
    static bool msg_destroy( int qid ) {
        return PK_Messages_global->destroy( qid );
    }
    static bool msg_send( int qid, pk_msg_int * msg ) {
        return PK_Messages_global->send( qid, msg );
    }
    static pk_msg_int * msg_recv( int num_qids, int * qids,
                                  int * retqid, int ticks ) {
        return PK_Messages_global->recv( num_qids, qids,
                                         retqid, ticks );
    }
    // semaphores - PK_Semaphores_global
    static PK_Semaphore * sem_create( char * name, ... ) {
        va_list ap;
        char tempn[100];
        va_start(ap,name);
        vsnprintf(tempn,sizeof(tempn)-1,name,ap);
        tempn[sizeof(tempn)-1] = 0;
        va_end(ap);
        return PK_Semaphores_global->create( tempn );
    }
    static PK_Semaphore * sem_lookup( char * name, ... ) {
        va_list ap;
        char tempn[100];
        va_start(ap,name);
        vsnprintf(tempn,sizeof(tempn)-1,name,ap);
        tempn[sizeof(tempn)-1] = 0;
        va_end(ap);
        return PK_Semaphores_global->lookup( tempn );
    }
    static void sem_destroy( PK_Semaphore * s ) {
        PK_Semaphores_global->destroy( s );
    }
    // timers - PK_Timers_global
    static int timer_create( int ticks, int qid, pk_msg_int * msg ) {
        return PK_Timers_global->create( ticks, qid, msg );
    }
    static int timer_create( int ticks, void (*func)(void *), void * arg ) {
        return PK_Timers_global->create( ticks, func, arg );
    }
    static int timer_create( int ticks, PK_Timeout_Obj * pkto ) {
        return PK_Timers_global->create( ticks, pkto );
    }
    static bool timer_cancel( int tid ) {
        return PK_Timers_global->cancel( tid );
    }
    static bool timer_cancel( int tid, pk_msg_int ** ptr, int * remaining ) {
        return PK_Timers_global->cancel( tid, ptr, remaining );
    }
    static int tps( void ) { return PK_Timers_global->tps(); }
    static void sleep( int ticks ) { return PK_Timers_global->sleep( ticks ); }
    static int get_time( void ) { return PK_Timers_global->get_time(); }
    static int /*pkfdid*/ register_fd( int fd, PK_FD_RW rw,
                                       int qid, void * obj ) {
        return PK_File_Descriptors_global->register_fd( fd, rw, qid, obj ); }
    static void * unregister_fd( int pkfdid ) {
        return PK_File_Descriptors_global->unregister_fd( pkfdid ); }
};

#endif /* __THREADS_H__ */
