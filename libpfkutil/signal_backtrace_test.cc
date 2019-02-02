
#include "signal_backtrace.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

using namespace BackTraceUtil;

void my_handler(void * arg, const SignalBacktraceInfo *info)
{
    ssize_t s = write(2, info->description, info->desc_len);
    if (s) { } // remove unused warning
    if (info->fatal)
        _exit(1);
}

int
main(int argc, char ** argv)
{
    if (argc != 2)
    {
        printf("usage: %s test_number\n", argv[0]);
        return 1;
    }

    int test = atoi(argv[1]);

    if (test == 1)
    {
        try {
            throw BackTrace();
        }
        catch (BackTrace bt) {
            printf("%s\n", bt.Format().c_str());
            exit (1);
        }
    }
    else if (test == 2)
    {
        SignalBacktrace::get_instance()->register_handler(
            "signal_test", NULL, &my_handler);

        SignalBacktrace::backtrace_now( "testing" );

        SignalBacktrace::cleanup();
    }
    else if (test == 3)
    {
        SignalBacktrace::get_instance()->register_handler(
            "signal_test", NULL, &my_handler);
        char * ptr = (char*) 1;
        *ptr = 0;
        SignalBacktrace::cleanup();
    }
    else if (test == 4)
    {
        SignalBacktrace::get_instance()->register_handler(
            "signal_test", NULL, &my_handler);
        printf("pid %d is sleeping now, send signal now\n", getpid());
        sleep(10);
        SignalBacktrace::cleanup();
    }

    return 0;
}
