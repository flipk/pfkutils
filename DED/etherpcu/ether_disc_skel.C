
#include "__ether_disc.H"
#include "network.H"
#include "timer.H"
#include "mproc.H"

extern "C"
int tickGet( void )
{
    return time_get();
}

int
ETHER_DISC_STATE_MACHINE :: input_discriminator( void * m )
{
    net_packet * p = (net_packet *) m;

    switch ( p->type )
    {
    case PKSTGEN_TIMER:
        return TIMEOUT_INPUT;
    case PACKET_BCI_S:
        return in_BCI_S;
    }

	return UNKNOWN_INPUT;
}

void
ETHER_DISC_STATE_MACHINE :: output_generator( int ty )
{
	// output messages based on 'ty' argument here
}

void
ETHER_DISC_STATE_MACHINE :: unknown_message( void * m )
{
    net_packet * p = (net_packet *) m;

    printf( PCUMPF "unknown message %#x\n", PCUMP, p->type );
}

void
ETHER_DISC_STATE_MACHINE :: unhandled_message( int type )
{
    printf( "unhandled message type '%s' in state '%s'\n",
            dbg_input_name( type ), dbg_state_name( current_state ));
}

void
ETHER_DISC_STATE_MACHINE :: debug_transition_hook( 
	int input, int old_state, int new_state )
{
	// nothing for now
}

void
ETHER_DISC_STATE_MACHINE :: set_timer( int val )
{
    timer_set( &timer_id, val, PKSTGEN_TIMER,
               info->mac, info->mac, seqno );
}

void
ETHER_DISC_STATE_MACHINE :: cancel_timer( void )
{
    timer_cancel( timer_id );
    timer_id.value = -1UL;
    seqno ++;
}

void
ETHER_DISC_STATE_MACHINE :: _first_call( void )
{
    current_state = MPROC_INIT1;
}
