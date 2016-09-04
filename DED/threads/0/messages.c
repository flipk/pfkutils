
#include "internal.h"

struct msg {
	int len;
	void *dat;
	struct msg *next;
};

struct msgq {
	int qid;
	int nummsgs;
	int owner;
	int waiter;
	mpoolset *pools;
	struct msg *first;
	struct msg *last;
	struct msgq *next;
};

int
msg_sizes( int which )
{
	switch (which)
	{
	case 0:
		return sizeof(struct msgq);
	case 1:
		return sizeof(struct msg);
	}

	return 0;
}

static struct msgq *qhash[MSG_HASH];

#define HASH(x) ((x) % MSG_HASH)

void
msg_init(void)
{
	int i;

	for (i=0; i < MSG_HASH; i++)
	{
		qhash[i] = NULL;
	}
}

__inline
static struct msgq *
locate_qid(qid, oq)
	int qid;
	struct msgq **oq;
{
	struct msgq *q;
	int h;

	h = HASH(qid);

	if (oq != NULL)
	{
		for (*oq = NULL, q = qhash[h];
		     q != NULL;
		     *oq = q, q = q->next)
		{
			if (q->qid == qid)
				break;
		}
	}
	else
	{
		for (q = qhash[h]; q != NULL; q = q->next)
		{
			if (q->qid == qid)
				break;
		}
	}

	return q;
}

/*
 * if qid already exists, return MSG_Q_EXISTS
 * if it does not exist, make new one.
 */
int
msg_register(qid, pools)
	int qid;
	mpoolset *pools;
{
	struct msgq *q, *oq, *new;

	q = locate_qid(qid, &oq);

	if (q != NULL)
	{
		fprintf(stderr,
			"msg_register failed: queue already exists!\n");
		return MSG_Q_EXISTS;
	}

	/* make new one */
	new = poolset_alloc(pools, sizeof(struct msgq));
	if (new == NULL)
	{
		fprintf(stderr, "msg_register failed: poolset_alloc failed\n");
		return MSG_NO_MEM;
	}

	new->qid = qid;
	new->nummsgs = 0;
	new->owner = th_tid();
	new->waiter = -1;
	new->pools = pools;
	new->first = NULL;
	new->last = NULL;
	new->next = NULL;

	if (oq == NULL)
	{
		qhash[HASH(qid)] = new;
	}
	else {
		oq->next = new;
	}

	return MSG_OK;
}

/*
 * if qid is found, return the mpoolset to the
 * user so it can be freed if desired.
 * return NULL if queue is not found.
 */
mpoolset *
msg_deregister(qid)
	int qid;
{
	struct msgq *q, *oq;
	struct msg *n, *nxt;
	mpoolset *ret;

	q = locate_qid(qid, &oq);
	if (q == NULL)
	{
		fprintf(stderr, "msg_deregister: msgq not found\n");
		return NULL;
	}

	if (q->owner != th_tid())
	{
		fprintf(stderr, "msg_deregister: not owner\n");
		return NULL;
	}

	if (oq == NULL)
	{
		qhash[HASH(qid)] = q->next;
	}
	else
	{
		oq->next = q->next;
	}

	for (n = q->first;
	     n != NULL;
	     n = nxt)
	{
		nxt = n->next;
		poolset_free(q->pools, n->dat);
		poolset_free(q->pools, n);
	}

	ret = q->pools;
	poolset_free(ret, q);

	return ret;
}

/*
 * return MSG_Q_NOT_EXIST if msgq doesn't exist;
 * return MSG_NO_MEM if msgq is full;
 * return 0 if success.
 */

int
msg_send(qid, buf, siz)
	int qid;
	char *buf;
	int siz;
{
	struct msgq *q;
	struct msg *m;

	q = locate_qid(qid, NULL);
	if (q == NULL)
	{
		return MSG_Q_NOT_EXIST;
	}

	m = poolset_alloc(q->pools, sizeof(struct msg));
	if (m == NULL)
	{
		return MSG_NO_MEM;
	}

	m->dat = poolset_alloc(q->pools, siz);
	if (m->dat == NULL)
	{
		poolset_free(q->pools, m);
		return MSG_NO_MEM;
	}

	m->len = siz;
	bcopy(buf, m->dat, siz);

	m->next == NULL;

	if (q->last == NULL)
	{
		q->first = q->last = m;
	}
	else
	{
		q->last->next = m;
		q->last = m;
	}
	q->nummsgs++;

	if (q->waiter != -1)
	{
		int tid = q->waiter;
		q->waiter = -1;
		th_resume(tid);
	}

	return MSG_OK;
}

__inline
static int
return_message(q, buf, siz)
	struct msgq *q;
	char *buf;
	int siz;
{
	struct msg *m;
	int ret;

	m = q->first;
	if (m->len > siz)
		return MSG_2BIG;

	q->nummsgs--;
	q->first = m->next;
	if (q->first == NULL)
	{
		q->last = NULL;
	}

	ret = m->len;
	bcopy(m->dat, buf, ret);

	poolset_free(q->pools, m->dat);
	poolset_free(q->pools, m);

	return ret;
}

/*
 * return MSG_Q_NOT_EXIST if queue does not exist
 * return MSG_2BIG if supplied buf is too small for packet
 * return MSG_TIMEOUT if timeout occurred
 * return size of successfully returned packet
 */

int
msg_recv(qid, buf, siz, ms)
	int qid;
	char *buf;
	int siz, ms;
{
	struct msgq *q;

	q = locate_qid(qid, NULL);
	if (q == NULL)
	{
		return MSG_Q_NOT_EXIST;
	}

	if (q->first == NULL)
	{
		if (ms < 0)
			return MSG_TIMEOUT;

		if (q->waiter != -1)
		{
			fprintf(stderr, 
				"msg_recv: tid %d is already waiting "
				"on qid %d!\n", q->waiter, qid);
			return MSG_BAD_ARG;
		}
		q->waiter = th_tid();

		if (ms == 0)
		{
			th_suspend(0);
			if ( q->first == NULL )
				return MSG_INTR;
		}
		else
		{
			int cc;

			cc = timer_sleep(ms);
			if ( cc == TIMER_OK )
				return MSG_TIMEOUT;

			if (( cc == TIMER_INTR ) && ( q->first == NULL ))
				return MSG_INTR;
		}
		/* the sender resets the waiter flag */
	}

	return return_message(q, buf, siz);
}

/*
 * return MSG_Q_NOT_EXIST if returned queue does not exist
 * return MSG_2BIG if supplied buf is too small for packet
 * return MSG_TIMEOUT if timeout occurred
 * return MSG_BAD_ARG if numqs is too big
 * if ticks is < 0, we're just polling, so return immediately with MSG_TIMEOUT
 * return size of successfully returned packet
 */

int
msg_recvlist(qids, numqs, qidret, buf, siz, ms)
	int *qids, numqs, *qidret, siz, ms;
	char *buf;
{
	struct msgq *qs[MAX_MSGQS];
	int i, tid, slotawoke;

	if (numqs > MAX_MSGQS)
	{
		fprintf(stderr,
			"msg_recvlist: can't wait on %d qids, MAX_MSGQS "
			"is only %d!\n",
			numqs, MAX_MSGQS);
		return MSG_BAD_ARG;
	}

	for (i=0; i < numqs; i++)
	{
		qs[i] = locate_qid(qids[i], NULL);
		if (qs[i]->first != NULL)
		{
			return return_message(qs[i], buf, siz);
		}
	}

	if (ms < 0)
	{
		return MSG_TIMEOUT;
	}

	tid = th_tid();

	for (i=0; i < numqs; i++)
	{
		if (qs[i]->waiter != -1)
		{
			fprintf(stderr, 
				"msg_recvlist: tid %d already waiting "
				"on msgq %d!\n", qs[i]->waiter, qids[i]);
			qs[i] = NULL;
		}
		else
		{
			qs[i]->waiter = tid;
		}
	}

	if (ms > 0)
	{
		int cc;

		cc = timer_sleep(ms);
		if ( cc == TIMER_OK )
			return MSG_TIMEOUT;

		if ( cc == TIMER_INTR )
			return MSG_INTR;
	}
	else
	{
		th_suspend(0);
	}

	slotawoke = -1;

	for (i=0; i < numqs; i++)
	{
		if ( qs[i] != NULL )
		{
			if ( qs[i]->first != NULL )
			{
				slotawoke = i;
				break;
			}
			qs[i]->waiter = -1;
		}
	}

	if (slotawoke == -1)
	{
		return MSG_INTR;
	}

	return return_message(qs[slotawoke], buf, siz);
}
