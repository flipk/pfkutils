
#include "internal.h"

#include <signal.h>
#include <setjmp.h>
#include <sys/time.h>

struct timer {
	int timerid;
	int ms;
	struct timer *next;
};

static struct timer *timer_head;
static mpool *pool;
static jmp_buf jb;
static int to_tt;

static void internal_timer_set(int timerid, int ms);
static void internal_timer_cancel(int timerid);
static void internal_timer_expire(int sig);

__inline
static void
setms(ms)
	int ms;
{
	struct itimerval v;
	
	v.it_interval.tv_sec  = 0;
	v.it_interval.tv_usec = 0;
	v.it_value.tv_sec  =  ms / 1000;
	v.it_value.tv_usec = (ms % 1000) * 1000;

	setitimer(ITIMER_REAL, &v, NULL);
}

__inline
static int
getms(void)
{
	struct itimerval v;

	getitimer(ITIMER_REAL, &v);

	return ((v.it_value.tv_sec  * 1000) +
		(v.it_value.tv_usec / 1000));
}

void
th_timer_process(infd, outfd)
	int infd, outfd;
{
	struct sigaction sa;
	struct itimerval v;
	struct timer_msg m;
	struct timer *t;
	int done, cc;
	sigset_t ss;

	to_tt = outfd;

	pool = pool_init(sizeof(struct timer), MAX_TIMERS);

	timer_head = NULL;

	sigemptyset(&ss);
	sigaddset(&ss, SIGALRM);
	sigprocmask(SIG_BLOCK, &ss, NULL);

	sa.sa_handler = internal_timer_expire;
	sigemptyset(&sa.sa_mask);
#ifdef __FreeBSD__
	sa.sa_flags = SA_RESTART | SA_NODEFER;
#endif
#ifdef sun
	sa.sa_flags = 0;
#endif
	sigaction(SIGALRM, &sa, NULL);

	for (done = 0; !done;)
	{
		if (setjmp(jb) == 0)
		{
			sigprocmask(SIG_UNBLOCK, &ss, NULL);
			cc = read(infd, &m, sizeof(m));
			sigprocmask(SIG_BLOCK, &ss, NULL);
		}
		else
		{ 
			/* a timer expired */
			t = timer_head;
			if (t == NULL)
				continue;
		expire:
			timer_head = t->next;
			m.timerid = t->timerid;
			m.msgtype = TIMER_EXPIRE;
			write(outfd, (char*)&m, sizeof(m));

			pool_free(pool, t);

			t = timer_head;
			if (t == NULL)
				continue;

			if (t->ms <= 0)
				goto expire;

			setms(t->ms);

			continue;
		}

		if (cc <= 0)
		{
			close(infd);
			close(outfd);
			return;
		}

		if (cc != sizeof(m))
		{
			fprintf(stderr, "timer_process: size mismatch.\n");
			fprintf(stderr, "cc = %d\n", cc);
			done = 1;
			break;
		}

		switch (m.msgtype)
		{
		case TIMER_SET:
			if (m.ms <= 0)
			{
				/* expire the timer now. */
				m.msgtype = TIMER_EXPIRE;
				write(outfd, (char*)&m, sizeof(m));
			}
			else
			{
				internal_timer_set(m.timerid, m.ms);
			}
			break;
		case TIMER_CANCEL:
			internal_timer_cancel(m.timerid);
			break;
		case TIMER_DIE:
			done = 1;
			break;
		default:
			fprintf(stderr, "timer_process: "
				"unexpected msg received.\n");
		}
	}

	m.msgtype = TIMER_DIE;
	write(outfd, &m, sizeof(m));
}

static void
internal_timer_expire(dum)
	int dum;
{
	longjmp(jb, 1);
}

static void
internal_timer_set(timerid, ms)
	int timerid;
	int ms; 
{
	struct timer *n, *t, *ot;
	int mstonext;

	n = pool_alloc(pool);
	if (n == NULL)
	{
		fprintf(stderr, "TIMER_SET failed: out of buffers\n");
		return;
	}

	n->timerid = timerid;
	n->ms = ms;

	if (timer_head == NULL)
	{
		n->next = NULL;
		timer_head = n;
		
		setms(ms);
		return;
	}

	/* else */
	mstonext = getms();

	if (mstonext >= ms)
	{
		timer_head->ms = mstonext - ms;
		n->next = timer_head;
		timer_head = n;
		setms(ms);
		return;
	}

	/* else */

	n->ms -= mstonext;

	for (ot = NULL, t = timer_head->next;
	     t != NULL;
	     ot = t, t = t->next)
	{
		if (t->ms > n->ms)
			break;

		n->ms -= t->ms;
	}

	n->next = t;

	if (ot != NULL)
	{
		ot->next = n;
	}
	else
	{
		timer_head->next = n;
	}

	if (t != NULL)
	{
		t->ms -= n->ms;
	}

	return;
}

static void
internal_timer_cancel(timerid)
	int timerid;
{
	struct timer *t, *ot;
	int mstogo;

	if (timer_head == NULL)
		return;

	if (timer_head->timerid == timerid)
	{
		mstogo = getms();
		t = timer_head;
		timer_head = t->next;
		if (timer_head != NULL)
		{
			timer_head->ms += mstogo;
			setms(timer_head->ms);
		} else {
			setms(0);
		}
		goto done;
	}

	for (ot = NULL, t = timer_head;
	     t != NULL;
	     ot = t, t = t->next)
	{
		if (t->timerid == timerid)
			break;
	}

	if (t == NULL)
		return;

	ot->next = t->next;
	if (t->next != NULL)
	{
		t->next->ms += t->ms;
	}

 done:
	pool_free(pool, t);
}
