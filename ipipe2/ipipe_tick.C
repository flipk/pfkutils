
#include <pthread.h>
#include <unistd.h>

#include "ipipe_tick.H"

struct thread_parms {
    int fd;
    struct timeval tv;
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
        struct timeval tv = parms->tv;
        select( 0, NULL, NULL, NULL, &tv );
    }
    close( parms->fd );
    delete parms;
    return 0;
}

tick_fd :: tick_fd( int tenths, void (*_func)(void*), void * _arg )
{
    pthread_attr_t    attr;
    thread_parms    * parms;
    pthread_t         newth;

    int pipe_ends[2];
    pipe( pipe_ends );

    func = _func;
    arg  = _arg;

    pthread_attr_init( &attr );
    pthread_attr_setdetachstate( &attr, 1 );

    parms = new thread_parms;

    fd                =  pipe_ends[0];
    parms->fd         =  pipe_ends[1];
    parms->tv.tv_sec  =  tenths / 10;
    parms->tv.tv_usec = (tenths % 10) * 100000;

    (void) pthread_create( &newth, &attr, tick_thread, (void*) parms );

    pthread_attr_destroy( &attr );
}

//virtual
tick_fd :: ~tick_fd( void )
{
    close( fd );
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
