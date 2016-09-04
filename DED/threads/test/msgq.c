#include "th.h"

void sender(void *);
void receiver(void *);

int
msgq_test()
{
	th_init();

	th_create(receiver, 0, 6, "recv");
	th_create(sender,   0, 5, "send");

	th_loop();

	return 0;
}

void
sender(dum)
	void *dum;
{
	char buf[16];
	int ret;
	int i;

	th_sleep(1);
	for (i=0; i < 100; i++)
	{
	  again:
		ret = msg_send(7277, buf, 16);
		if (ret == -2)
		{
			th_sleep(1);
			goto again;
		}
	}
}

int qr[] = { 101, 101 };
int sr[] = { 12, 32 };

void
receiver(dum)
	void *dum;
{
	mpoolset *pools;
	char buf[16];
	int ret;
	int i;

	pools = poolset_init(2, qr, sr);

	ret = msg_register(7277, pools);

	for (i=0; i < 100; i++)
	{
		ret = msg_recv(7277, buf, 16, 0);
	}

	pools = msg_deregister(7277);
	poolset_destroy(pools);
}
