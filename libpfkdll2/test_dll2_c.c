
/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
 */

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

DLL2_LIST  lists[DLL2_NUM_LISTS];
THING      a[NUMITEMS];

void
test_list(void)
{
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
        DLL2_ITEM_INIT( &a[i] );
        a[i].val = i;
    }

/* for this example we're placing these items on two lists,
   in different orders on each list */

    for ( i = 0; i < NUMITEMS; i++ )
    {
        DLL2_LIST_ADD( &lists[ DLL2_LIST_ONE ], &a[ i ] );
        DLL2_LIST_ADD( &lists[ DLL2_LIST_TWO ], &a[ i ] );
    }

    DLL2_LIST_REMOVE( &lists[ DLL2_LIST_ONE ], &a[ 4 ] );
    DLL2_LIST_ADD_AFTER(  &lists[ DLL2_LIST_ONE ], &a[ 5 ], &a[ 4 ] );

    DLL2_LIST_REMOVE( &lists[ DLL2_LIST_TWO ], &a[ 6 ] );
    DLL2_LIST_ADD_BEFORE( &lists[ DLL2_LIST_TWO ], &a[ 4 ], &a[ 6 ] );

/* and now, two examples of how to walk a list, one
   is from head to tail and the other is tail to head. */

#define WALKLIST_FORW(list) \
    for ( x = DLL2_LIST_HEAD(&lists[(list)]); x; \
          x = DLL2_LIST_NEXT(&lists[(list)],x))
#define WALKLIST_BACK(list) \
    for ( x = DLL2_LIST_TAIL(&lists[(list)]); x; \
          x = DLL2_LIST_PREV(&lists[(list)],x))

    printf( "\n" "list 1 (%d) head-to-tail: ",
            DLL2_LIST_SIZE(&lists[DLL2_LIST_ONE]));
    WALKLIST_FORW(DLL2_LIST_ONE)
        {
            printf( "%d ", x->val );
        }

    printf( "\n" "list 1 tail-to-head: " );
    WALKLIST_BACK(DLL2_LIST_ONE)
        {
            printf( "%d ", x->val );
        }

    printf( "\n\n" "list 2 (%d) head-to-tail: ",
            DLL2_LIST_SIZE(&lists[DLL2_LIST_TWO]));
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

    for ( i = 0; i < NUMITEMS; i++ )
    {
        for ( j = 0; j < DLL2_NUM_LISTS; j++ )
        {
            if ( DLL2_LIST_ONTHISLIST( &lists[ j ], &a[ i ] ))
                DLL2_LIST_REMOVE( &lists[ j ], &a[ i ] );
        }
        DLL2_ITEM_DEINIT( &a[i] );
    }

    DLL2_LIST_DEINIT( &lists[ DLL2_LIST_ONE ] );
    DLL2_LIST_DEINIT( &lists[ DLL2_LIST_TWO ] );
}

main()
{
    test_list();
    test_list();
    return 0;
}
