#include "internal.h"

void
th_timer_thread(dum)
	void *dum;
{
	struct fds *fds = (struct fds *)dum;
	int infd, outfd, i, done, cc;
	struct timer_msg m;

	infd = fds->infd;
	outfd = fds->outfd;

	free(fds);

	for (done = 0; !done;)
	{
		cc = th_read(infd, (char*)&m, sizeof(m));

		if (cc == 0)
		{
			fprintf(stderr, "timer_thread: connection to timer "
				"process was unexpectedly closed\n");
			break;
		}

		if (cc != sizeof(m))
		{
			fprintf(stderr, "timer_thread: size mismatch!\n");
			continue;
		}

		switch (m.msgtype)
		{
		case TIMER_EXPIRE:
			th_timer_expire(m.timerid);
			break;
		case TIMER_DIE:
			done = 1;
			break;
		default:
			fprintf(stderr, "unknown command received!\n");
			continue;
		}
	}
}
