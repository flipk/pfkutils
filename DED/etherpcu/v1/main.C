
#include "threads.H"
#include "creator.H"

int
main( int argc, char ** argv )
{
    if ( argc != 2 )
        return 1;

    srandom( time(0) * getpid() );

    int num_pcus = atoi( argv[1] );

    ThreadParams parms;

    parms.debug         = ThreadParams::DEBUG_PRINTSTACKINFO;
    parms.max_threads   = 50 + num_pcus * 3;
    parms.max_mqids     = 50 + num_pcus * 8;
    parms.timerhashsize = 50 + num_pcus * 4;
    parms.my_eid        = 1;

    Threads  th( &parms );

    new CreatorThread( num_pcus );
    th.loop();

    return 0;
}

extern "C"
int
tickGet( void )
{
    return ThreadShortCuts::tick();
}
