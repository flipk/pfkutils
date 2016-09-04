#include <stdio.h>
#include "th.h"

static void testfunc1(void *);
static void testfunc2(void *);

#if defined(__FreeBSD__)
#define RAND(x) (abs(arc4random() % (x)))
#elif defined(sun)
#define RAND(x) (random() % (x))
#endif

int
timer_main()
{
	int prio;

	setvbuf(stdout, NULL, _IOLBF, 0);

	prio = RAND(32);
	printf("starting main thread with prio of %d\n", prio);

	th_init();
	th_create(testfunc1, NULL, prio, "Test");
	th_loop();

	return 0;
}

int count;

static void
testfunc1(void *dum)
{
	count = 0;

	while (1)
	{
		printf("func1: waking up to spawn more threads. "
		       "there are %d right now\n", count);
		fprintf(stderr, 
			"func1: waking up to spawn more threads. "
		       "there are %d right now\n", count);

		while (count < 60)
		{
			printf("func1: making another thread.\n");
			th_create(testfunc2, NULL, RAND(32), "Test 2");
			count++;
			th_sleep(RAND(10));
		}

		th_sleep(RAND(300));
	}
}

static void
testfunc2(void *dum)
{
	int tid;
	int i, j;

	tid = th_tid();
	j = RAND(10);

	printf("thread %d: i am alive.\n", tid);

	for (i=0; i < j; i++)
	{
		th_sleep(RAND(60));
	}

	printf("thread %d: dying\n", tid);
	count--;
}
