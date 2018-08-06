
#include "signal_backtrace.h"
#include <unistd.h>
#include <stdio.h>

void my_handler(const SignalBacktraceInfo *info)
{
    ssize_t s = write(2, info->description, info->desc_len);
    if (s) { } // remove unused warning
    if (info->fatal)
        _exit(1);
}

int
main()
{
    SignalBacktrace::get_instance()->register_handler("signal_test",
                                                      &my_handler);

    SignalBacktrace::get_instance()->backtrace_now( "testing" );

    printf("pid %d is sleeping now, send signal now\n", getpid());
    sleep(10);

    char * ptr = (char*) 1;
    *ptr = 0;

    SignalBacktrace::cleanup();

    return 0;
}
