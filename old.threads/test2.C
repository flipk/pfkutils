
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>

#include "threads.H"

class testh : public Thread {
    static const int myprio = 10;
    static const int mystack = 32768;
    void entry( void );
    int inst;
public:
    testh( int _inst ) : Thread( "inst", myprio, mystack ) {
        inst = _inst;
        resume( tid );
    }
};

int
main()
{
    ThreadParams p;
    p.my_eid = 1;
    Threads th( & p );
    (void) new testh( 1 );
    (void) new testh( 2 );
    (void) new testh( 3 );
    th.loop();
    return 0;
}

ThreadSemaphore * sem;

void
testh :: entry( void )
{
    printf( "inst = %d\n", inst );
    if ( inst == 1 )
    {
        printf( "inst %d creates semaphore\n", inst );
        sem = th->sems->seminit( "testh", 1 );
    }
    printf( "inst %d taking sem\n", inst );
    printf( "inst %d take returns %d\n", inst, sem->take( ));
    printf( "inst %d sleeping\n", inst );
    sleep( tps() );
    printf( "inst %d awoke, giving sem\n", inst );
    sem->give();
    printf( "inst %d exits\n", inst );
    if ( inst == 3 )
    {
        printf( "inst %d deletes semaphore\n", inst );
        th->sems->semdelete( sem );
    }
}
