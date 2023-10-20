
#include "main.H"
#include "acceptor.H"

bool global_exit;

int
main( int argc, char ** argv )
{
    if ( argc != 2 )
    {
        printf( "usage: t <port>\n" );
        return 1;
    }

    int port = atoi( argv[1] );

    Threads th;
    global_exit = false;
    (void) new Acceptor( port );
    th.loop();

    return 0;
}
