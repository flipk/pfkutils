
#include "signal_backtrace.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>

using namespace BackTraceUtil;

void my_handler(void * arg, const SignalBacktraceInfo *info)
{
    ssize_t s = write(2, info->description, info->desc_len);
    if (s) { } // remove unused warning
    if (info->fatal)
        _exit(1);
}

static void *
signal_self(void *dummy)
{
    usleep(100000);
    kill(0, SIGTERM);
    return dummy;
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
        catch (const BackTrace &bt) {
            printf("%s\n", bt.Format().c_str());
            exit (1);
        }
    }
    else if (test == 2)
    {
        SignalBacktrace::get_instance()->register_handler(
            "signal_test", NULL, &my_handler);
        SignalBacktrace::clean_stack();
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
        pthread_t id;
        SignalBacktrace::get_instance()->register_handler(
            "signal_test", NULL, &my_handler);
        printf("sending SIGTERM to self\n", getpid());
        pthread_create(&id, NULL, &signal_self, NULL);
        sleep(10);
        SignalBacktrace::cleanup();
        void *dummy = NULL;
        pthread_join(id, &dummy);
    }

    return 0;
}
