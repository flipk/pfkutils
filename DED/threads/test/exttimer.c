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
exttimer_main()
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
		write(1, "R", 1);
		while (count < 60)
		{
			count++;
			th_create(testfunc2, NULL, RAND(32), "Test 2");
			timer_sleep(RAND(200));
		}

		write(1, "S", 1);
		timer_sleep(RAND(15000));
	}
}

static void
testfunc2(void *dum)
{
	int tid;
	int i, j;

	write(1, "c", 1);
	tid = th_tid();
	j = RAND(10);

	for (i=0; i < j; i++)
	{
		timer_sleep(RAND(5000));
	}

	count--;
	write(1, "d", 1);
}
