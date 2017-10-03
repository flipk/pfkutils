
#include "status.H"
#include "proxy.H"

extern void die( void );

statusThread :: statusThread( void )
    : Thread( "status", myprio, mystack )
{
    death_requested = false;
    resume( tid );
}

statusThread :: ~statusThread( void )
{
}

void
statusThread :: entry( void )
{
    proxyThread * p;
    char c;

    print_header( false );

    register_fd( 0 );
    while ( 1 )
    {
        c = 0;

        read( 0, &c, 1 );
        if ( death_requested )
            return;
        if ( c == 'q' )
            break;

        th->printinfo();

        p = proxys->iter_start();
        for ( ; p; p = p->next )
        {
            printf( "%d: (log %d) %s\n",
                    p->get_id(),
                    p->get_lognumber(),
                    p->my_url );
        }
        proxys->iter_done();
    }

    death_requested = true;
    ::die();
}
