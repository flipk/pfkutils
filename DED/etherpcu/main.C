
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "mproc.H"
#include "network.H"
#include "timer.H"

int              mproc_count;
pthread_mutex_t  mproc_count_mutex;
pthread_cond_t   mproc_count_condition;

int timer_exit_flag;

void *
mproc_thread( void * _arg )
{
    int          arg = (int) _arg;
    mproc_info   info;

    info.mac   = arg;
    info.pcu   = arg >> 1;
    info.mproc = arg & 1;

    activate_mproc( info.mac );
    mproc_main( &info );
    deactivate_mproc( info.mac );

    {
        pthread_mutex_lock ( &mproc_count_mutex );
        if ( --mproc_count == 1 )
            timer_exit_flag = 1;
        pthread_mutex_unlock ( &mproc_count_mutex );
    }

    pthread_exit( (void*) 0 );
}

void *
timer_thread( void * arg )
{
    pthread_mutex_lock ( &mproc_count_mutex );
    mproc_count ++;
    pthread_mutex_unlock ( &mproc_count_mutex );

    timer_exit_flag = 0;
    timer_main( &timer_exit_flag );

    pthread_mutex_lock ( &mproc_count_mutex );
    mproc_count--;
    pthread_mutex_unlock ( &mproc_count_mutex );
    pthread_cond_signal ( &mproc_count_condition );

    pthread_exit( (void*) 0 );
}

void *
supervisor_thread( void * _arg )
{
    int i, num_mprocs, num_pcus = (int) _arg;
    pthread_t            id;
    pthread_attr_t       thread_attr;
    pthread_condattr_t   cond_attr;
    pthread_mutexattr_t  mutex_attr;

    num_mprocs = num_pcus * 2;

    pthread_condattr_init     ( &cond_attr   );
    pthread_cond_init         ( &mproc_count_condition, &cond_attr );
    pthread_condattr_destroy  ( &cond_attr );
    pthread_mutexattr_init    ( &mutex_attr  );
    pthread_mutex_init        ( &mproc_count_mutex,     &mutex_attr );
    pthread_mutexattr_destroy ( &mutex_attr );

    mproc_count = 0;

    init_nets( num_mprocs );

    pthread_attr_init ( &thread_attr );

    pthread_create( &id, &thread_attr,
                    timer_thread, (void*) 0 );

    for ( i = 0; i < num_mprocs; i ++ )
    {
        pthread_mutex_lock ( &mproc_count_mutex );
        mproc_count ++;
        pthread_mutex_unlock ( &mproc_count_mutex );

        pthread_create ( &id, &thread_attr,
                         mproc_thread, (void*) i );
    }

    pthread_attr_destroy ( &thread_attr );

    pthread_mutex_lock ( &mproc_count_mutex );
    while ( mproc_count > 0 )
        pthread_cond_wait ( &mproc_count_condition, &mproc_count_mutex );
    pthread_mutex_unlock ( &mproc_count_mutex );

    net_cleanup();

    pthread_mutex_destroy ( &mproc_count_mutex );
    pthread_cond_destroy ( &mproc_count_condition );

    pthread_exit( (void*) 0 );
}


int
main( int argc, char ** argv )
{
    int              num_pcus;
    pthread_t        id;
    pthread_attr_t   attr;
    void           * ret;

    if ( argc != 2 )
    {
        printf( "usage: ./t <num_pcus>\n" );
        return 1;
    }

    srandom( time(0) * getpid() );

    num_pcus = atoi(argv[1]);

    if ( num_pcus < 1 || num_pcus > 300 )
    {
        printf( "num_pcus should be within 1-300\n" );
        return 2;
    }

    pthread_attr_init    ( &attr    );
    pthread_create       ( &id, &attr,
                           supervisor_thread, (void*)( num_pcus ) );
    pthread_attr_destroy ( &attr );
    pthread_join         ( id, &ret );

    return 0;
}
