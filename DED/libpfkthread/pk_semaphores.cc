/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/

#include "pk_semaphores.h"
#include "pk_timers.h"

/** \cond internal */
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
    while ((s = list.dequeue_head()) != NULL)
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
/** \endcond */

//

PK_Semaphore :: PK_Semaphore( const char * _name )
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
