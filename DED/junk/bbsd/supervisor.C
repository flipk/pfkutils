
#include "main.H"
#include "messages.H"
#include "supervisor.H"
#include "clients.H"

Supervisor * supervisor = NULL;

Supervisor :: Supervisor( int max_messages )
    : Thread( "Super", MY_PRIO )
{
    if ( supervisor != NULL )
    {
        stdio();
        printf( "Supervisor constructor: ERROR : only one "
                "supervisor may exist at a time!\n" );
        unstdio();
        global_msgs = NULL;
        clients = NULL;
        return;
    }

    global_msgs = new Messages( max_messages );
    clients = new Clients;
    stdio();
    printf( "Supervisor thread started.\n" );
    unstdio();
    supervisor = this;
    resume( tid );
}

Supervisor :: ~Supervisor( void )
{
    supervisor = NULL;
    if ( global_msgs != NULL )
        delete global_msgs;
    if ( clients != NULL )
        delete clients;
}

void
Supervisor :: entry( void )
{
    // this task used to do something useful.
    // it sort of devolved over time ... maybe i could
    // ditch this at some point, maybe roll it into
    // acceptor or something.

    while ( global_exit == false )
    {
        sleep( 10 );
    }

    // one useful thing this task can do is control the
    // shutdown of clients and clean up the message queue
    // data structures. this task does not exit until all
    // the clients have deregistered. this task's destructor
    // then cleans up the message queue and client list.

    while ( clients->get_num_clients() > 0 )
        sleep( 10 );

}

// the supervisor used to do this offline from the other
// tasks. then i decided, what's the point of having it
// move from one task to another when it doesn't have to.

void
Supervisor :: enqueue( char * dat, int size )
{
    Message * m;

    m = Message::newMessage( size );
    memcpy( m->data(), dat, size );

    while ( global_msgs->enqueue( m ) == false )
        sleep( 10 );
}

void
Supervisor :: status_function( void )
{
//  supervisor->global_msgs->status_function();
    supervisor->clients->status_function();
}
