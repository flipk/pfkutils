
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "mproc.H"
#include "network.H"
#include "timer.H"
#include "__ether_disc.H"

void
mproc_main( mproc_info * info )
{
    net_iface * iface = &mproc_nets[ info->mac ];
    net_packet * p;
    timer_ident   id;
    packet_type   type;

    printf( PCUMPF "powering on\n", PCUMP );

    timer_set( &id, 10 + (random() % 100), 
               PACKET_WAKEUP_INIT_TIME, info->mac, info->mac, 0 );

    while ( p = iface->receive( 100 ))
    {
        type = p->type;
        packet_free( p );
        if ( type == PACKET_WAKEUP_INIT_TIME )
            break;
    }

    PK_STATE_MACHINE_BASE * sm
        = new ETHER_DISC_STATE_MACHINE( "ether_disc", info );

    printf( PCUMPF "initialized\n", PCUMP );

    sm->first_call();

    while ( 1 )
    {
        PK_STATE_MACHINE_BASE::transition_return  ret;

        p = iface->receive( WAIT_FOREVER );
        ret = sm->transition( (void*) p );
        packet_free( p );
        if ( ret == PK_STATE_MACHINE_BASE::TRANSITION_EXIT )
            break;
    }

    delete sm;

    printf( PCUMPF "dead\n", PCUMP );
}
