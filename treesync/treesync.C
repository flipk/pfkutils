
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "treesync.h"
#include "trade.H"
#include "regen.H"

extern "C" int do_connect( char * host, int port );

int
main( int argc, char ** argv )
{
    int fd, port;
    char * host;
    bool first = false;

    srandom( time(NULL) * getpid() );

    switch ( argc )
    {
    case 1:
        host = NULL;
        port = -1;
        break;

    case 2:
        host = NULL;
        port = atoi(argv[1]);
        first = true;
        break;

    case 3:
        host = argv[1];
        port = atoi(argv[2]);
        break;

    default:
        printf( "\n"
                "usage: \n"
                "  ts                - regen db only (ONLY FIRST TIME!!!)\n"
                "  ts <port>         - regen, then wait for conn\n"
                "  ts <host> <port>  - regen, then connect to remote\n"
                "\n" );
        return 1;
    }

    if ( port != -1 )
    {
        fd = do_connect( host, port );
        trade_init( fd, first );
    }

    regenerate_database( true );

    if ( port == -1 )
    {
        clean_file_entry_list();
        return 0;
    }

    trade_files();
    clean_file_entry_list();
    trade_close(); // closes fd

    // after sync, regen again to account for files touched
    // during the sync itself.
    // xxx ntoe this assumes no files change during the time
    // the sync is in progress!

    regenerate_database( false );
    clean_file_entry_list();

    return 0;
}
