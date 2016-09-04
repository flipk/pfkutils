
#include "config.h"

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

/* pools */

#define MPOOLSETMAGIC 0xdeadbeed
#define MPOOLBUFMAGIC 0xdeadbeee
#define MPOOLMAGIC    0xdeadbeef

/* timers */

#define TIMER_SET      5
#define TIMER_CANCEL   6
#define TIMER_EXPIRE   7
#define TIMER_DIE      8

struct timer_msg {
	int msgtype;
	int timerid;
	int ms;
};

struct fds {
	int infd;
	int outfd;
};

void th_timer_process(int, int);
void th_timer_thread(void *);
void th_timer_kill(void);
void th_timer_expire(int);
