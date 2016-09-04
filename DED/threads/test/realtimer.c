
#include <stdio.h>
#include "th.h"

static void test_thread(void *);
static void test_thread2(void *);

int
realtimer_test(void)
{
	th_init();
	th_create(test_thread, 0, 5, "test");
	th_create(test_thread2, 0, 6, "test2");
	th_loop();

	return 0;
}

static void
test_thread(dum)
	void *dum;
{
	char buf[32];
	int t1, t2, t3, t4;

	t1 = timer_create(4000, 4, buf, 32, NULL);
	t2 = timer_create(3000, 4, buf, 32, NULL);
	t3 = timer_create(2000, 4, buf, 32, NULL);
	t4 = timer_create(1000, 4, buf, 32, NULL);
	timer_sleep(5000);
}

static int n = 3;
static int q[] = { 1, 10, 10 };
static int s[] = { MSGQ_SIZE, MSG_STRUCT_SIZE, 32 };

static void
test_thread2(dum)
	void *dum;
{
	mpoolset *pools;
	char buf[32];
	int cc;
	int i;

	pools = poolset_init(n, q, s);
	msg_register(4, pools);

	for (i=0; i < 4; i++)
	{
		cc = msg_recv(4, buf, 32, 0);
		printf("got msg of size %d on qid 4!\n", cc);
	}
}
