
#include "internal.h"

struct timer {
	int timerid;
	int tid;           /* tid to resume; -1 if we are to send msg */
	int qid;           /* msgq to send to; ignore if tid != -1 */
	char *msg;         /* points directly to user's msg to send */
	int len;           /* length of msg to send */
	mpoolset *pools;   /* when msg is sent, free msg to this pool */
	struct timer *next;
};

static struct timer *hash[TIMER_HASH];
static int next_timer_id;
static mpool *pool;
static int to_tp;

void
timer_init(void)
{
	int pid, i;
	int pipe1[2], pipe2[2];

	next_timer_id = 0;

	for (i=0; i < TIMER_HASH; i++)
		hash[i] = NULL;

	pool = pool_init(sizeof(struct timer), MAX_TIMERS);

	if (pipe(pipe1) < 0)
	{
		perror("to_tp pipe");
		exit(1);
	}

	if (pipe(pipe2) < 0)
	{
		perror("from_tp pipe");
		exit(1);
	}

	switch (pid = fork())
	{
	case -1:
		perror("fork timer process");
		exit(1);
	case 0:
		/* CHILD process */
		close(pipe1[1]);
		close(pipe2[0]);
		th_timer_process(pipe1[0], pipe2[1]);
		exit(0);
		/*NOTREACHED*/
	default:
		/* PARENT process */
		{
			struct fds *thfds;

			thfds = (struct fds *)malloc(sizeof(struct fds));

			thfds->infd  = pipe2[0];
			thfds->outfd = to_tp = pipe1[1];

			close(pipe1[0]);
			close(pipe2[1]);

			th_create(th_timer_thread, (void*)thfds, 62, "timer");
			break;
		} /* default */
	} /* switch */
}

void
th_timer_kill()
{
	struct timer_msg m;

	m.msgtype = TIMER_DIE;
	write(to_tp, &m, sizeof(m));
}

int
timer_create(ms, qid, msg, len, pools)
	int ms, len, qid;
	char *msg;
	mpoolset *pools;
{
	struct timer *t, *ot;
	struct timer_msg m;
	int h;

	while (1)
	{
		h = next_timer_id % TIMER_HASH;
		for (ot = NULL, t = hash[h];
		     t != NULL;
		     ot = t, t = t->next)
		{
			if (t->timerid == next_timer_id)
				break;
		}

		if (t == NULL)
			break;

		next_timer_id++;
	}

	t = pool_alloc(pool);
	if (t == NULL)
	{
		fprintf(stderr, "cannot set timer: out of buffers\n");
		return TIMER_NO_MEM;
	}

	m.msgtype = TIMER_SET;
	m.timerid = next_timer_id;
	m.ms = ms;
	write(to_tp, &m, sizeof(m));

	t->timerid = next_timer_id;
	t->tid = -1;
	t->qid = qid;
	t->msg = msg;
	t->len = len;
	t->pools = pools;
	t->next = NULL;

	if (ot == NULL)
	{
		hash[h] = t;
	}
	else
	{
		ot->next = t;
	}

	return next_timer_id++;
}

int
timer_cancel(timerid)
	int timerid;
{
	struct timer_msg m;
	struct timer *t, *ot;
	int h;

	h = timerid % TIMER_HASH;
	for (ot = NULL, t = hash[h];
	     t != NULL;
	     ot = t, t = t->next)
	{
		if (t->timerid == timerid)
			break;
	}

	if (t == NULL)
		return TIMER_FAIL;

	if (ot == NULL)
	{
		hash[h] = t->next;
	}
	else
	{
		ot->next = t->next;
	}

	if (t->tid == -1)
	{
		if (t->pools != NULL)
			poolset_free(t->pools, t->msg);
	}

	pool_free(pool, t);

	m.msgtype = TIMER_CANCEL;
	m.timerid = timerid;
	write(to_tp, &m, sizeof(m));

	return TIMER_OK;
}

void
th_timer_expire(timerid)
	int timerid;
{
	struct timer *t, *ot;
	int h;

	h = timerid % TIMER_HASH;
	for (ot = NULL, t = hash[h];
	     t != NULL;
	     ot = t, t = t->next)
	{
		if (t->timerid == timerid)
			break;
	}

	if (t == NULL)
	{
		return;
	}

	if (ot == NULL)
	{
		hash[h] = t->next;
	}
	else
	{
		ot->next = t->next;
	}

	if (t->tid != -1)
	{
		th_resume(t->tid);
	}
	else
	{
		msg_send(t->qid, t->msg, t->len);
		if (t->pools != NULL)
			poolset_free(t->pools, t->msg);
	}

	pool_free(pool, t);
}

int
timer_sleep(ms)
	int ms;
{
	struct timer *t, *ot;
	struct timer_msg m;
	int my_timerid;
	int h;

	if ( ms == 0 )
		return TIMER_OK;

	while (1)
	{
		h = next_timer_id % TIMER_HASH;
		for (ot = NULL, t = hash[h];
		     t != NULL;
		     ot = t, t = t->next)
		{
			if (t->timerid == next_timer_id)
				break;
		}

		if (t == NULL)
			break;

		next_timer_id++;
	}

	t = pool_alloc(pool);
	if (t == NULL)
	{
		fprintf(stderr, "cannot set timer: out of buffers\n");
		return TIMER_NO_MEM;
	}

	m.msgtype = TIMER_SET;
	my_timerid = m.timerid = next_timer_id;
	m.ms = ms;
	write(to_tp, &m, sizeof(m));

	t->timerid = next_timer_id;
	t->tid = th_tid();
	t->next = NULL;

	if (ot == NULL)
	{
		hash[h] = t;
	}
	else
	{
		ot->next = t;
	}

	next_timer_id++;
	th_suspend(0);

	/*
	 * if it wasn't th_timer_expire that resumed us,
	 * then the timer is probably still running.  Attempt to
	 * cancel the timer.  if the attempt to cancel the timer
	 * failed, that means the timer expired, so return that 
	 * timer_sleep worked as expected.  otherwise inform the
	 * caller that we were woken up some other way.
	 */

	if ( timer_cancel( my_timerid ) == TIMER_FAIL )
		return TIMER_OK;

	return TIMER_INTR;
}
