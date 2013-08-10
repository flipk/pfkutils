
#define STACK_SIZE 32768
#define MAX_THREADS 64

#define MAX_FDS 64
#define FD_SETSIZE MAX_FDS

#include <sys/types.h>
#include <sys/time.h>
#include <setjmp.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>

#include "th.h"

/* threads */

enum threadstate {
    TH_NONEXISTANT = 0,
    TH_READY    ,
    TH_SUSPENDED,
    TH_CURR     ,
    TH_SLEEP    ,
    TH_TSLEEP   ,
    TH_IOWAIT
};

typedef struct _thread {
    jmp_buf jb;
    char *stack;
    enum threadstate state;
    void (*func)();
    void *arg;
    int prio, tid, ticks;
    int *selectfds;
    int numselectfds, maxselectfds;
    char *name;
    struct _thread *next;
} thread_t;
