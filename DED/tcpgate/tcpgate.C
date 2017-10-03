
#include "threads.H"
#include "AcceptorThread.H"
#include "ConfigThread.H"
#include "DataThread.H"

extern "C" int tcpgate_main( int, char ** );
extern "C" int tcpgate_fast( int argc, char ** argv );

int
tcpgate_main( int argc, char ** argv )
{
    /* tcpgate -f 6001 10.0.0.9 6000 */
    if (((( argc - 2 ) % 3 ) == 0 )  &&
        strcmp( argv[1], "-f" ) == 0 )
    {
        return tcpgate_fast( argc, argv );
    }
    if ( argc != 1 )
    {
        fprintf( stderr,
                 "error in cmdline args\n"
                 "usage:  tcpgate    (with no args, reads ~/.tcpgate)\n"
                 "        tcpgate -f [port host port] [port host port]...\n" );
        return 1;
    }

    ConfigFile config;

    if ( ! config.isok())
        return 1;

    ThreadParams p;
    p.my_eid = 1;
    p.max_fds = 80;
    p.max_threads = 80;
    Threads th( &p );
    config.create_threads();
    th.loop();

    return 0;
}
