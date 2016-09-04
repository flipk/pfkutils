
#ifndef __TIMER_H__
#define __TIMER_H__

#include "network.H"

void timer_main( int * exit_flag );

union timer_ident {
    unsigned int value;
    struct {
        unsigned int  page     : 8;
        unsigned int  index    : 8;
        unsigned int  sequence :16;
    } fields;
};

// returns current tick number
int    time_get     ( void );

void   timer_list_dump ( int after );

// return -1 if failure or 0 if set
int    timer_set    ( timer_ident *, int ticks, net_packet * p );
int    timer_set    ( timer_ident *, int ticks, MSG_ARGS );
int    timer_set    ( timer_ident *, int ticks, void (*func)(void*), void * );
// return -1 if timer is already expired or 0 if cancelled
int    timer_cancel ( timer_ident );

#endif /* __TIMER_H__ */
