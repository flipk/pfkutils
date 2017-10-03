
#include "pk_threads.H"
#include "pk_threads_internal.H"

#include <stdio.h>
#include <unistd.h>

PK_Threads * th;

PK_Threads :: PK_Threads( int thread_hash_size,
                          int message_queue_hash_size,
                          int semaphore_hash_size,
                          int timer_hash_size )
{
    if ( th )
    {
        fprintf( stderr, "threads already running?!\n" );
        kill(0,6);
    }

    new PK_Message_Manager( message_queue_hash_size );
    new PK_Semaphores( semaphore_hash_size );
    new PK_Timer_Manager( timer_hash_size );
    thread_list = new PK_Thread_List( thread_hash_size );
    running = false;

    th = this;
}

PK_Threads :: ~PK_Threads( void )
{
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
    void * ret;
    running = true;
    while ( t = thread_list->get_head() )
        pthread_join( t->get_id(), &ret );
    running = false;
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
    bool send_sig = false;

    if ( th->running )
        pthread_yield();

    pthread_mutex_lock( &startup->mutex );
    if ( startup->waiting )
        send_sig = true;
    else
        startup->needed = false;
    pthread_mutex_unlock( &startup->mutex );
    if ( send_sig )
        pthread_cond_signal( &startup->cond );
}
