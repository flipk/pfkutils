/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

#include "pk_threads.H"
#include "pk_threads_internal.H"

#include <stdio.h>
#include <unistd.h>

PK_Threads * th;

PK_Threads :: PK_Threads( int _tps )
{
    if ( th )
    {
        fprintf( stderr, "threads already running?!\n" );
        kill(0,6);
    }

    new PK_Message_Manager;
    new PK_Semaphores;
    new PK_Timer_Manager( _tps );
    thread_list = new PK_Thread_List;
    running = false;
    pthread_cond_init( &runner_wakeup, NULL );
    th = this;
    new PK_File_Descriptor_Manager;
}

PK_Threads :: ~PK_Threads( void )
{
    delete PK_File_Descriptors_global;
    pthread_cond_destroy( &runner_wakeup );
    th = NULL;
    delete thread_list;
    delete PK_Timers_global;
    delete PK_Semaphores_global;
    delete PK_Messages_global;
}

void
PK_Threads :: add( PK_Thread * t )
{
    thread_list->add( t );
}

void
PK_Threads :: remove( PK_Thread * t )
{
    thread_list->remove( t );
    if ( thread_list->get_cnt() == 1 )
        PK_File_Descriptors_global->stop();
    if ( thread_list->get_cnt() == 0 )
        pthread_cond_signal( &runner_wakeup );
}

void
PK_Threads :: unhash_name( PK_Thread * t )
{
    thread_list->unhash_name( t );
}

void
PK_Threads :: rehash_name( PK_Thread * t )
{
    thread_list->rehash_name( t );
}

PK_Thread *
PK_Threads :: find_name( char * name )
{
    return thread_list->find_name( name );
}

PK_Thread *
PK_Threads :: find_id( pthread_t id )
{
    return thread_list->find_id( id );
}

void
PK_Threads :: run( void )
{
    PK_Thread * t;

    pthread_mutex_t  mut;
    pthread_mutex_init( &mut, NULL );
    running = true;

    // all threads existing at this point are sitting in the
    // initial wait-for-resume step.  so lets resume them all.

    for (t = thread_list->get_head(); t; t = thread_list->get_next(t))
        t->resume();

    // now, wait for all the threads to die.  whenever any thread
    // dies, the condition is poked.  when the last thread dies, 
    // we return to the caller.

    while ( thread_list->get_cnt() > 0 )
        pthread_cond_wait( &runner_wakeup, &mut );

    running = false;
    pthread_mutex_destroy( &mut );
}

//

PK_Thread :: PK_Thread( void )
{
    startup = new startup_sync;

    pthread_cond_init( &startup->cond, NULL );
    pthread_mutex_init( &startup->mutex, NULL );
    startup->waiting = false;
    startup->needed = true;

    name = strdup( "_thread_XXXXXXXX" );
    pthread_create( &id, NULL, _entry, (void*) this );
    sprintf( name+8, "%08x", (unsigned int) id );
    th->add( this );
    pthread_detach( id );
}

//static
void *
PK_Thread :: _entry( void * _pk_thread )
{
    PK_Thread * t = (PK_Thread *) _pk_thread;

    pthread_mutex_lock( &t->startup->mutex );
    if ( t->startup->needed )
    {
        // wait for resume() called
        t->startup->waiting = true;
        pthread_cond_wait( &t->startup->cond, &t->startup->mutex );
    }
    pthread_mutex_unlock( &t->startup->mutex );

    pthread_mutex_destroy( &t->startup->mutex );
    pthread_cond_destroy( &t->startup->cond );

    delete t->startup;
    t->startup = NULL;

    t->entry();
    th->remove( t );
    delete t;
    return NULL;
}

void
PK_Thread :: resume( void )
{
    if (!th->running)
        // resume does nothing, because th->run() is not started yet;
        // when th->run() is started, all existing threads will be
        // resumed.
        return;

    bool send_sig = false;

    pthread_mutex_lock( &startup->mutex );
    if ( startup->waiting )
        send_sig = true;
    else
        startup->needed = false;
    pthread_mutex_unlock( &startup->mutex );

    if ( send_sig )
        pthread_cond_signal( &startup->cond );
}
