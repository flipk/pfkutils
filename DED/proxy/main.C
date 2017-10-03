
/*
export MALLOC_OPTIONS=AJ
*/

#include "proxy.H"
#include "conn.H"
#include "status.H"

#include <signal.h>

void die( void );
void info( void );

static connThread * conn_thread_instance;
static statusThread * status_thread_instance;

extern "C" int webproxy_main( void );

int
webproxy_main( void )
{
    ThreadParams p;

    p.my_eid = 1;
    p.max_threads = 128;
    p.max_fds = 128;

#if 0
    signal( SIGUSR1, (void(*)(int))&die );
    signal( SIGINT, (void(*)(int))&die );
#endif
    signal( SIGPIPE, SIG_IGN );
    signal( SIGINFO, (void(*)(int))&info );

    Threads th( &p );
    conn_thread_instance = new connThread;
    status_thread_instance = new statusThread;

    proxys = new linked_list<proxyThread>( "proxys" );
    th.loop();

    return 0;
}

void
die( void )
{
    proxyThread * p;
    status_thread_instance->die();
    p = proxys->iter_start();
    for ( ; p; p = p->next )
        p->die();
    proxys->iter_done();
    delete proxys;
    conn_thread_instance->die();
}

void
info( void )
{
    th->printinfo();
}
