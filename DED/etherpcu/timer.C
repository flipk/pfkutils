
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "timer.H"
#include "network.H"
#include "dll2.H"

struct timer_frame {
    LListLinks <timer_frame> links[1];
    int           ordered_queue_key;
//
    int           ticks;
    timer_ident   ident;
    net_packet  * p;
    void        (*func)( void * );
    void        * arg;
};

struct timer_frame_page {
    static const int frames_per_page = 256;
    timer_frame   tfs[ frames_per_page ];
};

pthread_mutex_t   timer_mutex;

static inline void
lock( void )
{
    pthread_mutex_lock( &timer_mutex );
}

static inline void
unlock( void )
{
    pthread_mutex_unlock( &timer_mutex );
}

LList          <timer_frame,0>   timers_free;
LListOrderedQ  <timer_frame,0>   timers;

int                  num_timer_frame_pages;
timer_frame_page  *      timer_frame_pages[ 256 ];

// static
timer_frame *
timer_alloc( void )
{
    timer_frame * ret;
    lock();
    if ( timers_free.get_cnt() == 0 )
    {
        int page_num = num_timer_frame_pages++;
        timer_frame_page * page = new timer_frame_page;
        timer_frame_pages[ page_num ] = page;
        for ( int i = 0; i < 256; i++ )
        {
            timer_frame * tf = &page->tfs[i];
            tf->ident.fields.page     = page_num;
            tf->ident.fields.index    = i;
            tf->ident.fields.sequence = 1;
            timers_free.add( tf );
        }
    }
    ret = timers_free.dequeue_head();
    unlock();
    return ret;
}

// static
void
timer_free( timer_frame * tf )
{
    lock();
    tf->ident.fields.sequence ++;
    timers_free.add( tf );
    unlock();
}


int
timer_set    ( timer_ident * id, int ticks, net_packet * p )
{
    timer_frame * tf;

    tf = timer_alloc();
    tf->ticks = ticks;
    tf->p = p;
    tf->func = NULL;
    tf->arg = NULL;
    if ( id )
        id->value = tf->ident.value;
    p->extra = tf->ident.value;

    timers.add( tf, ticks );

    return 0;
}

int 
timer_set    ( timer_ident * id, int ticks, MSG_ARGS )
{
    net_packet * p = packet_alloc( type, src, dest, extra );
    return timer_set( id, ticks, p );
}

int
timer_set    ( timer_ident * id, int ticks,
               void (*func)(void*), void * arg )
{
    timer_frame * tf;

    tf = timer_alloc();
    tf->ticks = ticks;
    tf->p = NULL;
    tf->func = func;
    tf->arg = arg;
    if ( id )
        id->value = tf->ident.value;

    timers.add( tf, ticks );

    return 0;
}


void
timer_list_dump( int after )
{
    timer_frame * tf;
    lock();
    if ( after )
        printf( "--> " );
    else
        printf( "    " );
    printf( "timer list: " );
    for ( tf = timers.get_head(); tf; tf = timers.get_next(tf) )
    {
        printf( "%d-%d ",
                tf->ticks, tf->ordered_queue_key );
    }
    printf( "\n" );
    unlock();
}

int
timer_cancel ( timer_ident id )
{
    timer_frame * tf;

    tf = &timer_frame_pages[ id.fields.page ]->tfs[ id.fields.index ];

    if ( timers.onthislist( tf ))
    {
        if ( tf->ident.fields.sequence != id.fields.sequence )
            return -1;

        timers.remove( tf );
        if ( tf->p )
            packet_free( tf->p );
        timer_free( tf );

        return 0;
    }

    return -1;
}

int tick_number;

int
time_get( void )
{
    return tick_number;
}


void
timer_main( int * exit_flag )
{
    int i;
    num_timer_frame_pages = 0;
    for ( i = 0; i < 256; i++ )
        timer_frame_pages[i] = NULL;

    pthread_mutexattr_t          mtxattr;
    pthread_mutexattr_init    ( &mtxattr );
    pthread_mutex_init        ( &timer_mutex, &mtxattr );
    pthread_mutexattr_destroy ( &mtxattr );

    tick_number = 0;

    timer_frame * tf;

    while ( *exit_flag == 0 )
    {
        usleep( 100000 );

        tick_number ++;

//        timer_list_dump(0);

        lock();
        tf = timers.get_head();
        if ( tf )
            tf->ordered_queue_key --;
        while ( tf = timers.get_head() )
        {
            if ( tf->ordered_queue_key > 0 )
                break;
            timers.remove( tf );
            unlock();
            if ( tf->p )
            {
                packet_send( tf->p );
                tf->p = NULL;
            }
            else
            {
                tf->func( tf->arg );
            }
            timer_free( tf );
            lock();
        }
        unlock();

//        timer_list_dump(1);
    }

    // cleanup

    while ( tf = timers.get_head() )
        timers.remove( tf );
    while ( tf = timers_free.dequeue_head() )
        ;
    for ( i = 0; i < 256; i++ )
        if ( timer_frame_pages[i] )
            delete timer_frame_pages[i];

}
