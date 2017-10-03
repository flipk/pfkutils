
#include <stdlib.h>
#include "scan.H"
#include "trade.H"
#include "trade_messages.H"

extern "C" int do_connect( char * host, int port );

int
main( int argc, char ** argv )
{
    int fd;
    bool first;

    if ( argc == 2 )
    {
        first = false;
        fd = do_connect( NULL, atoi(argv[1]) );
    }
    else if ( argc == 3 )
    {
        first = true;
        fd = do_connect( argv[1], atoi(argv[2]) );
    }
    else
    {
        fprintf( stderr, "usage: tsn <port> or tsn <host> <port>\n" );
        return 1;
    }

    printf( "connection established\n" );

    tcp_chan = new tsn_tcp_chan( fd );

    {
        MsgScanStarted  start;
        tcp_chan->send( &start );
        if ( tcp_chan->recv( &start, sizeof(start) ) == false )
        {
            fprintf( stderr, "error negotiating link\n" );
            return 1;
        }
    }

    scan_start();

    {
        MsgScanComplete comp;
        tcp_chan->send( &comp );
        if ( tcp_chan->recv( &comp, sizeof(comp) ) == false )
        {
            fprintf( stderr, "error negotiating link\n" );
            return 1;
        }
    }

    bool success;
    if ( first )
        success = trade_first();
    else
        success = trade_second();

    scan_finish(success);

    return (success)?0:1;
}
