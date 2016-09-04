
#include "threads.H"
#include "creator.H"
#include "ether.H"
#include "hsc.H"
#include "mproc.H"

#if 0
#define PROCLIST "99proclist"
#endif

void
CreatorThread :: entry( void )
{
    printf( "creator started\n" );

    ether = new ethernet( pcus * 3 );

    for ( int cage = 1; cage <= pcus; cage++ )
    {
        MPROC_Thread * m1, * m2;
        char threadname[50];

        sprintf( threadname, "c%d_m7", cage );
        m1 = new MPROC_Thread( cage, 7, threadname );
        sprintf( threadname, "c%d_m9", cage );
        m2 = new MPROC_Thread( cage, 9, threadname );

        hsc * h = new hsc( cage, m1->get_hsc_mqid(), m2->get_hsc_mqid() );

        m1->wakeup( m2, h );
        m2->wakeup( m1, h );
    }

#ifdef PROCLIST
    while ( get_numthreads() > 6 )
    {
        sleep( tps() * 3 );
        FILE * f = fopen( PROCLIST, "w" );
        th->printinfo2(f);
        fclose(f);
    }
#endif
}
