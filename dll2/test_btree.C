#if 0
set -x -e
g++ -g3 -c test_btree.C
g++ -g3 -c dll2_hash.C
g++ -g3 test_btree.o dll2_hash.o -o tb
exit 0
#endif

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#define DLL2_CHECKSUMS      0
#define DLL2_INCLUDE_BTREE  1
#include "dll2.H"

/*
  #define BTORDER 13
  #define MAX 1000000
  with these parameters we get
   - 149000 inserts per second
   - 243902 lookups per second
*/

struct thing {
    LListBTREELink btree_link;
    int btree_key( void ) { return v; }
    thing( int _v ) { v = _v; inlist = false; }
    int v;
    bool inlist;
};

#if 1

#define NUMS 1

#if NUMS==1
#define BTORDER 13
#define MAX     500000
#define REPS    5000000
#elif NUMS==2
#define BTORDER 5
#define MAX     500000
#define REPS    50000000
#endif

typedef LListBTREE<thing,BTORDER> BT;

thing ** a;

int
main()
{
    BT * bt;
    int i, j, s;
    time_t last, now;

    s = getpid() * time(0);
    s = -2141375889;
    srandom( s );

//    printf( "S %d\n", s );

    time( &last );
    a = new thing*[ MAX ];

    for ( i = 0; i < MAX; i++ )
        a[i] = new thing( i );

    bt = new BT;

    for ( i = 0; i < REPS; i++ )
    {
        j = random() % MAX;

        time( &now );
        if ( now != last )
        {
            printf( "reps %d\n", i );
            last = now;
        }

        if ( a[j]->inlist )
        {
            if ( bt->find( j ) == 0 )
                printf( "not found 1\n" );
//            printf( "remove %d\n", j );
            bt->remove( a[j] );
        }
        else
        {
//            printf( "add %d\n", j );
            bt->add( a[j] );
            if ( bt->find( j ) == 0 )
                printf( "not found 1\n" );
        }

        a[j]->inlist = !a[j]->inlist;

//        bt->printtree();
    }

    for ( i = 0; i < MAX; i++ )
    {
        if ( a[i]->inlist )
            bt->remove( a[i] );
        delete a[i];
    }
    delete[] a;

    bt->printtree();

    delete bt;

    fflush(stdout);
    fflush(stderr);

    return 0;
}
#endif

#if 0
#define BTORDER 5
#define MAX 50
typedef LListBTREE<thing,BTORDER> BT;

int vals[MAX];

int
main()
{
    BT bt;
    thing * a;
    int i;
    struct timeval tv[5];
    double tvfl[5];

    srandom( time(0) * getpid());

    gettimeofday( tv+0, 0 );
    for ( i = 0; i < MAX; i++ )
        vals[i] = i;
    for ( i = 0; i < MAX; i++ )
    {
        int j = random() % MAX;
        int t = vals[i];
        vals[i] = vals[j];
        vals[j] = t;
    }

    gettimeofday( tv+1, 0 );
    for ( i = 0; i < MAX; i++ )
        bt.add( new thing( vals[i] ));
    gettimeofday( tv+2, 0 );
    for ( i = 0; i < MAX; i++ )
    {
        int j = random() % MAX;
        int t = vals[i];
        vals[i] = vals[j];
        vals[j] = t;
    }
    gettimeofday( tv+3, 0 );
    for ( i = 0; i < MAX; i++ )
    {
        a = bt.find( vals[i] );
        if ( !a || a->v != vals[i] )
            printf( "not found\n" );
    }
    gettimeofday( tv+4, 0 );

    for ( i = 0; i < 5; i++ )
        tvfl[i] =
            (double)(tv[i].tv_sec) + 
            ((double)(tv[i].tv_usec) / (double)1000000);

    for ( i = 1; i < 5; i++ )
        printf( "%5.2f\n", tvfl[i] - tvfl[i-1]);

    return 0;
}
#endif
