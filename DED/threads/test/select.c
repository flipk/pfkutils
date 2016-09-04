#include <stdio.h>

#include "th.h"

static void testfunc1(void *dum);
static void testfunc2(void *dum);

int
select_main()
{
	th_init();
	th_create(testfunc1, NULL, 5, "Test1");
	th_create(testfunc2, NULL, 5, "Test2");
	th_loop();

	return 0;
}

volatile int done = 0;

static void
testfunc1(dum)
	void *dum;
{
	char data[4096];
	int rfds[1];
	int ofds[1];
	int cc, y = 0;
	int t = 0;
	int c = 0;

	rfds[0] = 0;

	cc = 1;
	while (cc > 0)
	{
		while (th_select(1, rfds, 0, NULL, 1, ofds, 0) == 0)
		{
			y++;
			th_yield();
		}

		cc = read(0, data, 4096);
		t += cc;
		c++;
	}

	done = 1;
	printf("read %d bytes in %d reads, yielded %d times\n", t, c, y);
}

static void
testfunc2(dum)
	void *dum;
{
	int i = 0;

	while (!done)
	{
		i++;
		th_yield();
	}

	printf("i is %d\n", i);
}
