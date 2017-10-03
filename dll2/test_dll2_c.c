#if 0
set -ex
gcc -g3 -c dll2_c.c
gcc -g3 -c test_dll2_c.c
gcc -g3 test_dll2_c.o dll2_c.o -o t
./t
exit 0
#endif

#include "dll2_c.h"

enum { DLL2_LIST_ONE, DLL2_LIST_TWO, DLL2_NUM_LISTS };

typedef struct {
    int val;
    DLL2_LINKS links[DLL2_NUM_LISTS];
} THING;

/* this example code puts NUMITEMS items on each list */
#define NUMITEMS 10

int
main()
{
    DLL2_LIST  lists[DLL2_NUM_LISTS];
    THING      a[NUMITEMS];
    THING * x;
    int i,j;

/* here is how you initialize a DLL2_LIST --
   you have to tell each list what 'index' to use within
   the 'links' array within the items */

    DLL2_LIST_INIT( &lists[ DLL2_LIST_ONE ], DLL2_LIST_ONE );
    DLL2_LIST_INIT( &lists[ DLL2_LIST_TWO ], DLL2_LIST_TWO );

/* for this example, we allocate and initialize 
   NUMITEMS items.  you must use the DLL2_LINKS_INIT macro
   anytime an 'item' is created, to initialize the 'links' array. */

    for ( i = 0; i < NUMITEMS; i++ )
    {
        DLL2_LINKS_INIT( &a[i] );
        a[i].val = i;
    }

/* for this example we're placing these items on two lists,
   in different orders on each list */

    for ( i = 0; i < NUMITEMS; i++ )
    {
        if ( i != 4 )
            DLL2_ADD( &lists[ DLL2_LIST_ONE ], &a[ i ] );
        if ( i != 6 )
            DLL2_ADD( &lists[ DLL2_LIST_TWO ], &a[ i ] );
    }

    DLL2_ADD_AFTER(  &lists[ DLL2_LIST_ONE ], &a[ 5 ], &a[ 4 ] );
    DLL2_ADD_BEFORE( &lists[ DLL2_LIST_TWO ], &a[ 4 ], &a[ 6 ] );

/* and now, two examples of how to walk a list, one
   is from head to tail and the other is tail to head. */

#define WALKLIST_FORW(list) \
    for ( x = DLL2_HEAD(&lists[(list)]); x; \
          x = DLL2_NEXT(&lists[(list)],x))
#define WALKLIST_BACK(list) \
    for ( x = DLL2_TAIL(&lists[(list)]); x; \
          x = DLL2_PREV(&lists[(list)],x))

    printf( "\n" "list 1 head-to-tail: " );
    WALKLIST_FORW(DLL2_LIST_ONE)
        {
            printf( "%d ", x->val );
        }
    printf( "\n" "list 1 tail-to-head: " );
    WALKLIST_BACK(DLL2_LIST_ONE)
        {
            printf( "%d ", x->val );
        }
    printf( "\n\n" "list 2 head-to-tail: " );
    WALKLIST_FORW(DLL2_LIST_TWO)
        {
            printf( "%d ", x->val );
        }
    printf( "\n" "list 2 tail-to-head: " );
    WALKLIST_BACK(DLL2_LIST_TWO)
        {
            printf( "%d ", x->val );
        }
    printf( "\n\n" );

    return 0;
}
