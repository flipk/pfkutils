#if 0
set -e -x
g++ -c thread_states.cc
g++ -c thread_states_test.cc
g++ -c thread_states_print.cc
g++ thread_states.o thread_states_test.o -o thread_states_test
g++ thread_states.o thread_states_print.o -o thread_states_print
exit 0
;
#endif

#include "thread_states.h"
#include <unistd.h>
#include <stdio.h>

int main()
{
    thread_states_init();

    while (1)
    {
        THSTATE_LINE(thread1_lineno);
        THSTATE_BUMP(thread1_iterations);
        THSTATE_SET(thread1_situation1,  1);
        usleep(100);

        THSTATE_LINE(thread1_lineno);
        THSTATE_SET(thread1_situation1,  2);
        usleep(100);

        THSTATE_LINE(thread1_lineno);
        THSTATE_SET(thread1_situation1,  0);
        THSTATE_SET(thread1_situation2,  1);
        usleep(100);

        THSTATE_LINE(thread1_lineno);
        THSTATE_SET(thread1_situation2,  2);
        usleep(100);

        THSTATE_LINE(thread1_lineno);
        THSTATE_SET(thread1_situation2,  3);
        usleep(100);

        THSTATE_LINE(thread1_lineno);
        THSTATE_SET(thread1_situation2,  0);
        usleep(100);
    }

    return 0;
}
