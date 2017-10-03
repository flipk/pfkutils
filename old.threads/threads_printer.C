
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <signal.h>

#include "threads.H"
#include "threads_internal.H"
#include "threads_messages_internal.H"
#include "threads_timers_internal.H"
#include "threads_printer_internal.H"

// wierd thing happens on freebsd. if you put fd 0
// in nonblocking mode, then fd 1 goes into nonblocking
// mode too (its not a per-fd thing, its a per-device thing,
// and since fds 0 and 1 are the same device what changes one
// changes the other).

// this probably isn't freebsd-specific.

void
ThreadPrinter :: entry( void )
{
    if ( register_mq( mqid, "printer" ) == false )
        ::printf( "failure registering printer mq!\n" );

    register_fd( 2 );
    die = false;

    while ( !die )
    {
        union {
            Message * m;
            PrintMessage * pm;
        } m;

        m.m = recv( 1, &mqid, NULL, WAIT_FOREVER );
        if ( m.m == NULL )
            continue;

        if ( m.m->type.get() == PrintMessage::TYPE )
        {
            th->write( 2,
                       m.pm->printbody,
                       m.pm->len.get() );
        }

        delete m.m;
    }
}
