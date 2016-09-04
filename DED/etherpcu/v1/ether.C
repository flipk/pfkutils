
#include <sys/types.h>
#include <signal.h>

#define INCLUDE_ETHER_TYPE_NAMES
#include "ether.H"

#define DEBUG 0

ethernet * ether;

ethernet :: ethernet( int _max_mqs )
{
    max_mqs = _max_mqs;
    mqs = new int[ max_mqs ];
    nummqs = 0;
}

int
ethernet :: register_mac( int mqid )
{
    int ret = nummqs;
    mqs[ret] = mqid;
    nummqs++;
    return ret;
}

void
ethernet :: unregister_mac( int mac )
{
    mqs[mac] = -1;
}

//static
const char *
ethernet :: type_name( eth_msg_type type )
{
    return ether_type_names[type];
}

void
ethernet :: tx( int src_mac, int dest_mac, eth_msg_type type )
{
#if DEBUG
    printf( "transmitting %s from mac %d to mac %d\n",
            type_name(type), src_mac, dest_mac );
#endif

    if ( dest_mac == -1 )
    {
        ::fprintf( stderr, "error in ethernet::tx, directed to broadcast\n" );
        kill(0,6);
    }

    if ( mqs[dest_mac] == -1 )
    {
        // target is offline, drop message
        printf( "msg %s to mac %d discarded\n",
                type_name(type), dest_mac );
        return;
    }

    ETHERNET_MSG * m = new ETHERNET_MSG;

    m->dest.set( mqs[dest_mac] );
    m->src_mac = src_mac;
    m->dest_mac = dest_mac;
    m->ether_type = type;

    if ( send( m, &m->dest ) == false )
    {
        ::fprintf( stderr, "error in ethernet::tx send\n" );
        kill(0,6);
    }
}

void
ethernet :: tx( int src_mac, eth_msg_type type )
{
#if DEBUG
    printf( "broadcasting %s from mac %d to mac %d\n",
            type_name(type), src_mac, -1 );
#endif

    for ( int i = 0; i < nummqs; i++ )
    {
        if ( i == src_mac   ||
             mqs[i] == -1   )
            continue;

        ETHERNET_MSG * m = new ETHERNET_MSG;

        m->dest.set( mqs[i] );
        m->src_mac = src_mac;
        m->dest_mac = -1;
        m->ether_type = type;

        if ( send( m, &m->dest ) == false )
        {
            ::fprintf( stderr, "error in ethernet::tx2 send\n" );
            kill(0,6);
        }
    }
}
