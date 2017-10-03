
#include <pthread.h>
#include <unistd.h>

#include "ipipe_tick.H"

struct thread_parms {
    int fd;
    int us;
};

static void *
tick_thread( void * ptr )
{
    thread_parms * parms = (thread_parms *) ptr;

    while ( 1 )
    {
        char c = 1;
        if ( write( parms->fd, &c, 1 ) != 1 )
            break;
        struct timeval tv = { parms->us / 1000000, parms->us % 1000000 };
        select( 0, NULL, NULL, NULL, &tv );
    }
    delete parms;
    return 0;
}

tick_fd :: tick_fd( int tenths )
{
    pthread_attr_t  thread_attributes;
    thread_parms * parms;

    int pipe_ends[2];
    pipe( pipe_ends );

    func = NULL;

    pthread_attr_init( &thread_attributes );
    pthread_attr_setdetachstate( &thread_attributes, 1 );

#if 1
    parms = new thread_parms;

    parms->fd = pipe_ends[1];
    parms->us = tenths * 100000;

    pthread_t        new_thread;
    pthread_create( &new_thread, &thread_attributes,
                    tick_thread, (void*) parms );
#endif

    pthread_attr_destroy( &thread_attributes );

    fd = pipe_ends[0];
}

//virtual
tick_fd :: ~tick_fd( void )
{
    close( fd );
}

void
tick_fd :: register_tick( void (*_func)(void*), void * _arg )
{
    func = _func;
    arg  = _arg;
}

//virtual
bool
tick_fd :: select_for_read ( fd_mgr * mgr )
{
    return true;
}

//virtual
fd_interface :: rw_response
tick_fd :: read ( fd_mgr * mgr )
{
    char buf[10];
    int cc;

    cc = ::read( fd, buf, 10 );

    if ( cc <= 0 )
        return DEL;

    if ( func )
        func( arg );

    return OK;
}

//virtual
bool
tick_fd :: select_for_write( fd_mgr * mgr )
{
    return false;
}

//virtual
fd_interface :: rw_response
tick_fd :: write( fd_mgr * )
{
    // nothing
    return DEL;
}
