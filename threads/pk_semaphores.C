
#include "pk_semaphores.H"
#include "pk_timers.H"

PK_Semaphores * PK_Semaphores_global;

PK_Semaphores :: PK_Semaphores( void )
{
    if ( PK_Semaphores_global )
    {
        fprintf( stderr, "PK_Semaphores already initialized?!\n" );
        kill(0,6);
    }
    pthread_mutex_init( &mutex, NULL );
    PK_Semaphores_global = this;
}

PK_Semaphores :: ~PK_Semaphores( void )
{
    PK_Semaphore * s;
    while ( s = list.dequeue_head() )
    {
        fprintf( stderr, "while destroying semaphore manager: "
                 "deleted stale semaphore '%s'\n",
                 s->get_name() );
        hash.remove( s );
        delete s;
    }
    PK_Semaphores_global = NULL;
    pthread_mutex_destroy( &mutex );
}

//

PK_Semaphore :: PK_Semaphore( char * _name )
{
    pthread_mutex_init( &mutex, NULL );
    pthread_cond_init( &cond, NULL );

    value = 1;
    waiters = 0;
    name = strdup( _name );
}

PK_Semaphore :: ~PK_Semaphore( void )
{
    pthread_mutex_destroy( &mutex );
    pthread_cond_destroy( &cond );
    free( name );
}

bool
PK_Semaphore :: take( int ticks )
{
    _lock();
    while ( value == 0 )
    {
        if ( ticks == 0 )
        {
            _unlock();
            return false;
        }
        PK_Timeout_Obj pkto( cond );
        int tid = -1;
        if ( ticks > 0 )
            tid = PK_Timers_global->create( ticks, &pkto );
        waiters++;
        pthread_cond_wait( &cond, &mutex );
        waiters--;
        if ( pkto.timedout )
        {
            _unlock();
            return false;
        }
        if ( tid != -1 )
            PK_Timers_global->cancel( tid );
    }
    value--;
    _unlock();
    return true;
}

void
PK_Semaphore :: give( void )
{
    bool sig = false;
    _lock();
    value++;
    if ( waiters )
        sig = true;
    _unlock();
    if ( sig )
        pthread_cond_signal( &cond );
}
