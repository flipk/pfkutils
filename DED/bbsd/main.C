
/*
    - "DCC" whatever that is.
 */


#include "main.H"
#include "acceptor.H"
#include "supervisor.H"

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
    supervisor = new Supervisor( 1024 );
    (void) new Acceptor( port );
    th.status_function = Supervisor::status_function;
    th.loop();

    return 0;
}
