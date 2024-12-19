
#include "thread_states.h"
#include <stdio.h>

int main()
{
    thread_states_init(/*init_contents*/ false);

#define THREAD_STATES_ITEM(item)  printf( #item " = %d\n", thstates->item) ;
    THREAD_STATES_LIST ;
#undef  THREAD_STATES_ITEM

    return 0;
}
