
#include <pthread.h>
#include <sys/time.h>
#include <errno.h>

#include "network.H"
#include "timer.H"

char * packet_names[] = {
    "BCI-S",
    "BCI-F"
};

static int num_mprocs;
net_iface * mproc_nets;
hsc_controller * hscs;

net_iface :: net_iface( void )
{
    pthread_condattr_t   cond_attr;
    pthread_mutexattr_t  mutex_attr;

    pthread_condattr_init     ( &cond_attr );
    pthread_cond_init         ( &cond, &cond_attr );
    pthread_condattr_destroy  ( &cond_attr );

    pthread_mutexattr_init    ( &mutex_attr );
    pthread_mutex_init        ( &mtx, &mutex_attr );
    pthread_mutexattr_destroy ( &mutex_attr );

    active = false;
}

struct netreceive_wakeup {
    pthread_cond_t * cond;
    int timed_out;
};

static void
netreceive_wakeup_func( void * arg )
{
    netreceive_wakeup * parms = (netreceive_wakeup *) arg;

    parms->timed_out = 1;
    pthread_cond_signal( parms->cond );
}

net_packet *
net_iface :: receive( int timeout )
{
    net_packet * ret;
    struct timespec  abstime;

    lock();
    if ( pkts.get_cnt() == 0  &&  timeout == 0 )
    {
        unlock();
        return NULL;
    }
    netreceive_wakeup   parms;
    timer_ident         tid;
    
    parms.timed_out = 0;

    if ( timeout > 0 )
    {
        parms.cond = &cond;
        timer_set( &tid, timeout,
                   netreceive_wakeup_func, (void *) &parms );
    }
    while ( pkts.get_cnt() == 0 )
    {
        pthread_cond_wait( &cond, &mtx );
        if ( parms.timed_out )
            break;
    }
    if ( timeout > 0  &&  !parms.timed_out )
        timer_cancel( tid );
    ret = pkts.dequeue_head();
    unlock();
    return ret;
}

void
net_iface :: send( net_packet * pkt )
{
    if ( !active )
        return;
    lock();
    pkts.add( pkt );
    unlock();
    pthread_cond_signal( &cond );
}

void
net_iface :: activate( void )
{
    active = true;
}

void
net_iface :: deactivate( void )
{
    net_packet * p;
    active = false;
    lock();
    while ( p = pkts.dequeue_head() )
    {
        unlock();
        packet_free( p );
        lock();
    }
    unlock();
}

hsc_controller :: hsc_controller( void )
{
    pthread_mutexattr_t     attr;

    pthread_mutexattr_init    (  &attr );
    pthread_mutex_init        ( &mtx, &attr );
    pthread_mutexattr_destroy ( &attr );

    domains[0] = HSC_FREE;
    domains[1] = HSC_FREE;
}

pthread_mutex_t  freelist_mutex;
LList <net_packet,0>  packet_freelist;
int packets_total;
int packets_free;
int packets_inuse;

net_packet *
packet_alloc( void )
{
    net_packet * ret;

    pthread_mutex_lock( &freelist_mutex );
    packets_inuse ++;
    ret = packet_freelist.dequeue_head();
    if ( ret )
        packets_free --;
    else
        packets_total ++;
    pthread_mutex_unlock( &freelist_mutex );

    if ( ret == NULL )
        ret = new net_packet;

    return ret;
}

net_packet *
packet_alloc( MSG_ARGS )
{
    net_packet * ret = packet_alloc();

    ret->type  = type;
    ret->src   = src;
    ret->dest  = dest;
    ret->extra = extra;

    return ret;
}

net_packet *
packet_alloc( net_packet * oldp )
{
    net_packet * ret = packet_alloc();

    ret->type  = oldp->type;
    ret->src   = oldp->src;
    ret->dest  = oldp->dest;
    ret->extra = oldp->extra;

    return ret;
}

void
packet_free( net_packet * p )
{
    pthread_mutex_lock( &freelist_mutex );
    packet_freelist.add( p );
    packets_inuse --;
    packets_free ++;
    pthread_mutex_unlock( &freelist_mutex );
}

void
init_nets( int _num_mprocs )
{
    num_mprocs      = _num_mprocs;
    mproc_nets      = new net_iface[ _num_mprocs ];
    hscs            = new hsc_controller[ _num_mprocs / 2 ];

    packets_total   = 0;
    packets_free    = 0;
    packets_inuse   = 0;

    pthread_mutexattr_t          mutex_attr;
    pthread_mutexattr_init    ( &mutex_attr );
    pthread_mutex_init        ( &freelist_mutex, &mutex_attr );
    pthread_mutexattr_destroy ( &mutex_attr );
}

void
packet_send( MSG_ARGS )
{
    packet_send( packet_alloc( type, src, dest, extra ));
}

void
packet_send( net_packet * p )
{
    if ( p->dest == -1 ) // broadcast
    {
        int i;
        net_packet * np;
        for ( i = 0; i < num_mprocs; i++ )
        {
            if ( i != p->src )
            {
                np = packet_alloc( p );    // copy body
                mproc_nets[ i ].send( np );
            }
        }
        packet_free( p );
    }
    else
    {
        mproc_nets[ p->dest ].send( p );
    }
}

void
activate_mproc( int mac )
{
    mproc_nets[ mac ].activate();
}

void
deactivate_mproc( int mac )
{
    mproc_nets[ mac ].deactivate();
}

void
net_cleanup( void )
{
    int i;
    net_packet * p;
    for ( i = 0; i < num_mprocs; i++ )
        mproc_nets[i].deactivate();
    while ( p = packet_freelist.dequeue_head() )
        delete p;
}
