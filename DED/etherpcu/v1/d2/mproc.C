
#include "mproc.H"
#include "hsc.H"
#include "ether.H"
#include "ether_disc.H"

#define DEBUG 1

MPROC_Thread :: MPROC_Thread( int _cage, int _slot, char * _name )
    : Thread( _name, 5, 32768 )
{
    slot = _slot;

    int nlen = strlen( _name ) + 1;
    name = new char[ nlen ];
    memcpy( name, _name, nlen );

    char z[64];
    sprintf( z, "%s_h", name );
    if ( register_mq( mqids[MQ_HSC], z ) == false )
        printf( "unable to register mqid for hsc\n" );
    sprintf( z, "%s_e", name );
    if ( register_mq( mqids[MQ_ETHER], z ) == false )
        printf( "unable to register mqid for ether\n" );
    sprintf( z, "%s_t", name );
    if ( register_mq( mqids[MQ_TIMER], z ) == false )
        printf( "unable to register mqid for ether\n" );
    sprintf( z, "%s_d", name );
    sm = new ETHER_DISC_STATE_MACHINE( z, this );

    mac = ether->register_mac( mqids[MQ_ETHER] );
    haslink = false;
    // do not resume tid, creator does that after hooking up the hsc.
}

MPROC_Thread :: ~MPROC_Thread( void )
{
    delete[] name;
    delete sm;
    unregister_mq( mqids[MQ_HSC   ] );
    unregister_mq( mqids[MQ_ETHER ] );
    unregister_mq( mqids[MQ_TIMER ] );
    ether->unregister_mac( mac );
}

void
MPROC_Thread :: entry( void )
{
    ETHER_DISC_INPUT_MSG edim;
    union {
        Message * m;
        HSC_OTHER_ATTEMPTED_CLAIM * hoac;
        HSC_OTHER_RELEASED        * hor;
        HSC_RESET_PLEASE          * hrp;
        ETHERNET_MSG              * em;
        ETHER_DISC_TIMER          * edt;
    } m;
    int mqout;

 start_over:

    sm->first_call();
    while ( 1 )
    {
        m.m = recv( MQ_N, mqids, &mqout, NO_WAIT );
        if ( m.m == NULL )
            break;
        delete m.m;
    }
    printf( "initializing\n" );
    int s = 40 + (random() % 5);
    sleep( s * tps() );

 start_over_without_delay:

    sm->first_call();

    if ( h->take( slot, &take_bcast, this ) == false )
    {
        printf( "i am secondary\n" );
        edim.type = ETHER_DISC_INPUT_MSG::BECOME_SECONDARY;
        sm->transition( &edim );
    }
    else
    {
        printf( "i am primary\n" );
    }

    whattodo = CONTINUE;
    while ( whattodo == CONTINUE )
    {
        m.m = recv( MQ_N, mqids, &mqout, WAIT_FOREVER );
        if ( m.m == NULL )
        {
            printf( "null message received!\n" );
            continue;
        }

        if ( mqout == mqids[MQ_HSC] )
        {
            switch ( m.m->type.get() )
            {
            case HSC_OTHER_ATTEMPTED_CLAIM::TYPE:
                printf( "other mproc attempted claim\n" );
                edim.type = ETHER_DISC_INPUT_MSG::C1_ASSERTION;
                sm->transition( &edim );
                break;
            case HSC_OTHER_RELEASED::TYPE:
                printf( "other mproc released claim\n" );
                whattodo = RESTART2;
                break;
            case HSC_RESET_PLEASE::TYPE:
                whattodo = RESTART1;
                break;
            default:
                printf( "unknown message 1 of type %#x received\n",
                        m.m->type.get() );
            }
        }
        else if ( mqout == mqids[MQ_TIMER] )
        {
            edim.type            = ETHER_DISC_INPUT_MSG::TIMER_INDICATION;
            edim.sequence_number = m.edt->sequence_id;
            sm->transition( &edim );
        }
        else if ( mqout == mqids[MQ_ETHER] )
        {
            if ( m.m->type.get() == ETHERNET_MSG::TYPE )
            {
                switch ( m.em->ether_type )
                {
                case E_CLAIM:
                case E_CLAIM_ACK:
                case E_CLAIM_NMM:
                case E_HEARTBEAT:
                case E_HEARTBEAT_ACK:
#if DEBUG
                    printf( "got %s from mac %d (i am %d)\n",
                            ethernet::type_name( m.em->ether_type ),
                            m.em->src_mac, mac );
#endif
                case E_START_BCAST:
                case E_FINISH_BCAST:

                    edim.type          = ETHER_DISC_INPUT_MSG::IML_INDICATION;
                    edim.ether_type    = m.em->ether_type;
                    edim.ether_src_mac = m.em->src_mac;
                    sm->transition( &edim );
                    break;
                default:
                    printf( "unknown ethernet type message %#x received\n",
                            m.em->ether_type );
                }
            }
            else
            {
                printf( "unknown message 2 of type %#x received\n",
                        m.m->type.get() );
            }
        }
        delete m.m;
    }

    if ( whattodo == RESTART1 )
        goto start_over;
    else if ( whattodo == RESTART2 )
        goto start_over_without_delay;

    // sleep a couple seconds before dying to give the other thread
    // a chance to collect link information and verify it connected
    // to the correct mproc.

    sleep( tps() * 2 );
}

#undef iml_print

void
MPROC_Thread :: iml_print( char * format, ... )
{
    char line[ 500 ];
    int l;
    va_list ap;
    va_start( ap, format );
    l = vsnprintf( line, 499, format, ap );
    line[499] = 0;
    va_end( ap );
    printf( "%s\n", line );
}

int
MPROC_Thread :: set_ether_timer( int ticks, int seq )
{
    ETHER_DISC_TIMER * edt = new ETHER_DISC_TIMER;
    edt->dest.set( mqids[MQ_TIMER] );
    edt->sequence_id = seq;
    return set( ticks, edt );
}

//static
void
MPROC_Thread :: take_bcast( void * arg, hsc::hsc_take_bcast_type type )
{
    MPROC_Thread * m = (MPROC_Thread *)arg;

    switch ( type )
    {
    case hsc::BCAST_START:
        ether->tx( m->mac, E_START_BCAST );
        sleep(1);
        ether->tx( m->mac, E_START_BCAST );
        sleep(1);
        ether->tx( m->mac, E_START_BCAST );
        sleep(1);
        break;
    case hsc::BCAST_FINISH:
        sleep(15);
        ether->tx( m->mac, E_FINISH_BCAST );
        sleep(1);
        ether->tx( m->mac, E_FINISH_BCAST );
        sleep(1);
        ether->tx( m->mac, E_FINISH_BCAST );
        break;
    }
}

void
MPROC_Thread :: restart_audit( void )
{
    h->reset_other( slot );
}
