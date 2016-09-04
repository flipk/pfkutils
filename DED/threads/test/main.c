#include <stdio.h>

int timer_main(void);
int select_main(void);
int pooltest_main(void);
int msgq_test(void);
int realtimer_test(void);
int exttimer_test(void);

int
main(argc, argv)
	int argc;
	char ** argv;
{
	if (argc != 2)
	{
	  printerr:
		printf("usage: t [1 | 2 | 3 | 4 | 5]\n");
		printf("         1: test select timers\n");
		printf("         2: test select\n");
		printf("         3: test pools\n");
		printf("         4: test msgqs\n");
		printf("         5: test basic timers\n");
		printf("         6: test extended timers\n");
		printf("	 7: test extended messages\n");
		exit(1);
	}

#ifndef __FreeBSD__
	srandom(time(NULL) * getpid());
#endif

	switch (atoi(argv[1]))
	{
	case 1:
		return timer_main();
	case 2:
		return select_main();
	case 3:
		return pooltest_main();
	case 4:
		return msgq_test();
	case 5:
		return realtimer_test();
	case 6:
		return exttimer_main();
	case 7:
		return msgq2_test();
	default:
		goto printerr;
	}

	return 0;
}
