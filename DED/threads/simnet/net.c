#include <stdio.h>

#include "th.h"
#include "simnet.h"

#define NUMHEADERS 10500

static mpool * packet_header_pool;

#define NEWH()   pool_alloc(packet_header_pool)
#define FREEH(x) pool_free(packet_header_pool, x)

/*
 * simulate transmitting packets, 
 * receiving packets, and waiting for them.
 * also uses the topology functions to determine
 * the length of the path from one node to another.
 */

struct packet {
	char *data;
	int length;
	int from;
	int to;
	int delay;
	int current_delay;
	int time_sent;
	struct packet *next;
};

/* map node names to task ids */
static int node_taskid[MAX_NODES];
static int node_is_waiting[MAX_NODES];

/*
 * these are the packet queues for each node;
 * insert into the tail, remove from the head.
 */
static struct packet *queue_head[MAX_NODES];
static struct packet *queue_tail[MAX_NODES];

/*
 * this is the output queue; since it takes time for data to
 * get sent over a link, a packet lives on this list until it
 * is actually time for it to be sent.  this list is sorted by
 * age .. head is the smallest time left, tail is largest.
 * therefore insertion must go that way -- walk the list until
 * the appropriate place is found.
 */

static struct packet *outq;

static void  net_deliver_packet(struct packet *);

void
net_init(void)
{
	int i;

	packet_header_pool = pool_init(sizeof(struct packet), NUMHEADERS);

	for (i=0; i < MAX_NODES; i++)
	{
		queue_head[i] = queue_tail[i] = NULL;
		node_taskid[i] = -1;
		node_is_waiting[i] = FALSE;
	}

	outq = NULL;
}

void
net_destroy(void)
{
	struct packet *p, *q;
	int i;

	for (i=0; i < MAX_NODES; i++)
	{
		net_destroy_node(i);
	}

	for (p = outq; p != NULL; p = q)
	{
		q = p->next;
		/* this is actually incorrect, since the node2
		   algorithm uses its own pool for data */
		free(p->data);
		FREEH(p);
	}
}


void
net_init_node(int myname)
{
	node_taskid[myname] = th_tid();
	queue_head[myname] = queue_tail[myname] = NULL;
}

void
net_destroy_node(int myname)
{
	struct packet *p, *q;

	node_taskid[myname] = -1;
	node_is_waiting[myname] = FALSE;

	for (p = queue_head[myname];
	     p != NULL;
	     p = q)
	{
		q = p->next;
		/* same comment as in net_destroy */
		free(p->data);
		FREEH(p);
	}

	queue_head[myname] = queue_tail[myname] = NULL;
}

char *
net_receive(int *buflen, int *from, int to)
{
	char *buf;
	struct packet *p;

	if (queue_head[to] == NULL)
	{
		node_is_waiting[to] = TRUE;
		th_suspend(0);
		node_is_waiting[to] = FALSE;
	}

	if (queue_head[to] == NULL)
		return NULL;

	p = queue_head[to];
	queue_head[to] = p->next;
	buf = p->data;
	*buflen = p->length;
	*from = p->from;

	if (p->next == NULL)
		queue_tail[to] = NULL;

	FREEH(p);

	messages_received ++;

	return buf;
}

void
net_send(char *buf, int buflen, int from, int to)
{
	int pathlen;
	struct packet *p, *t, *ot;

	p = NEWH();

	if (p == NULL)
	{
		printf("net_send: out of buffers\n");
		exit(1);
	}

	/* first look up the length of the target path. */
	pathlen = topology_path_length(from, to);

	p->data = buf;
	p->length = buflen;
	p->from = from;
	p->to = to;
	p->delay = pathlen;
	p->time_sent = clock_time_now();

	for (ot = NULL, t = outq;
	     t != NULL;
	     ot = t, t = t->next)
	{
		if (t->current_delay > pathlen)
			break;
		pathlen -= t->current_delay;
	}

	p->current_delay = pathlen;

	if (ot)
	{
		p->next = ot->next;
		ot->next = p;
		if (t != NULL)
			t->current_delay -= pathlen;
		return;
	}

	outq = p;
	p->next = t;
	if (t != NULL)
		t->current_delay -= pathlen;

	return;
}

/*
 * return 1 when the process should exit.
 */
int
net_clock_isr(void)
{
	struct packet *p;

	/*
	 * if the queue is empty coming into the clock isr,
	 * it means that nothing is going to happen next round,
	 * so we might as well bail out.
	 */

	if (outq == NULL)
	{
		int i;

		simulator_exit = TRUE;

		for (i=0; i < MAX_NODES; i++)
		{
			if (node_taskid[i] != -1)
				th_resume(node_taskid[i]);
		}

		return 1;
	}

	outq->current_delay--;

	/*
	 * process every packet on head of list who is ready.
	 */

	while (1)
	{
		if (!outq)
			break;

		p = outq;

		if (p->current_delay > 0)
			break;

		outq = p->next;

		net_deliver_packet(p);
	}

	return 0;
}

static void
net_deliver_packet(struct packet *p)
{
	int to;

	to = p->to;
	p->next = NULL;

	if (queue_tail[to] == NULL)
	{
		queue_head[to] = queue_tail[to] = p;
	}
	else
	{
		queue_tail[to]->next = p;
		queue_tail[to] = p;
	}

	if (node_is_waiting[to] == TRUE)
	{
		node_is_waiting[to] = FALSE;
		th_resume(node_taskid[to]);
	}
}
