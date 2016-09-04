/*
 * optimal route discovery
 *
 * in this example, the node which receives the "start" command (which
 * is node zero) begins an optimal route discovery algorithm, sending out
 * pings to the neighbors to discover who their neighbors are, etc.  As
 * each packet returns, the length of the shortest path to be discovered
 * is updated.  When the packet flurry dies off, the node which
 * originated the exchange now knows the absolute best path to take to
 * get to that destination.
 *
 * each and every node performs this algorithm.
 *
 * turns out this takes 156964 messages for the 18-node "map 2" in map.h.
 */

#include "th.h"
#include "simnet.h"

#define NUMPACKETS 10500

struct net_packet {
#define NET_PACKET_MAGIC 0x4e455421
	int magic;
	int source;
	int destination;
	enum net_packet_type {
		PACKET_DISCOVER = 1,
		PACKET_DISCOVER_REPLY = 2
	} type;
	int time_sent;
	int path[MAX_NODES];
};

static mpool * packet_pool;

#define NEWP()   pool_alloc(packet_pool)
#define FREEP(x) pool_free(packet_pool, x)

typedef struct net_packet P;

struct node_info {
	int myname;
	int nodes;
	int node_path[MAX_NODES][MAX_NODES];
	unsigned int path_length[MAX_NODES];
	struct node_identity *nip;
};

static int numnodes;

static void
injector_task(void *dummy)
{
	char *buf;
	int i;

	for (i = 0; i < numnodes; i++)
	{
		buf = NEWP();
		strcpy(buf, "start");
		net_send(buf, 6, i, i);
	}
}

void
inject_initial_packet()
{
	numnodes = 0;
	packet_pool = pool_init(sizeof(P), NUMPACKETS);
	th_create(injector_task, 0, 2, "injector");
}

#define PLEASE_FREE 1
#define DONT_FREE   0

static void start_route_discovery        (struct node_info *info);
static int  handle_packet_discover       (struct node_info *info, P *p);
static int  handle_packet_discover_reply (struct node_info *info, P *p);

void
node_main(void *myname0)
{
	struct node_identity *nip;
	struct node_info info;
	int bufsize, from, i, j;
	char *buf;

	numnodes ++;

	nip = (struct node_identity *)myname0;
	net_init_node(nip->myname);

	info.nip = nip;
	info.myname = nip->myname;
	info.nodes  = 0;

	for (i = 0; i < MAX_NODES; i++)
	{
		/* don't know the route */
		for (j = 0; j < MAX_NODES; j++)
		{
			info.node_path[i][j] = -1;
		}

		/* can't get there from here */
		info.path_length[i] = 0xffffffff;
	}

	buf = NULL;

	while (1)
	{
		P *p;

		if (buf != NULL)
			FREEP(buf);

		buf = net_receive(&bufsize, &from, nip->myname);

		if (buf == NULL)
			break;

		if ((bufsize == 6) && (strncmp(buf, "start", 5) == 0))
		{
			start_route_discovery(&info);
			continue;
		}

		if (bufsize < sizeof(int))
		{
			printf("%d: packet too small; dropping\n",
			       nip->myname);
			continue;
		}

		p = (P *) buf;

		if (p->magic != NET_PACKET_MAGIC)
		{
			printf("%d: magic number mismatch; dropping\n",
			       nip->myname);
			continue;
		}

		if (p->type == PACKET_DISCOVER)
		{
			if (handle_packet_discover(&info, p) == DONT_FREE)
				buf = NULL;
			continue;
		}

		if (p->type == PACKET_DISCOVER_REPLY)
		{
			if (handle_packet_discover_reply(&info, p) 
			    == DONT_FREE)
				buf = NULL;
			continue;
		}

		printf("%d: unknown packet type %d received\n",
		       nip->myname, p->type);
	}

	if (buf != NULL)
		FREEP(buf);

	printf("\n");
	printf("NODE %d ROUTING INFORMATION:\n", info.myname);

	for (i = 0; i < MAX_NODES; i++)
	{
		if (info.node_path[i][0] == -1)
			continue;
		printf("path to %d: ", i);
		for (j = 0; j < MAX_NODES; j++)
		{
			if (info.node_path[i][j] == -1)
				break;
			printf("%d ", info.node_path[i][j]);
		}
		printf("\n");
	}

	printf("\n");

	net_destroy_node(nip->myname);
}

static void
start_route_discovery(struct node_info *info)
{
	int i, j;
	P * p;

	for (i = 0; i < info->nip->neighbors; i++)
	{
		p = NEWP();
		if (p == NULL)
		{
			printf("out of buffers\n");
			exit(1);
		}
		p->magic = NET_PACKET_MAGIC;
		p->source = info->myname;
		p->destination = info->nip->myneighbors[i];
		p->type = PACKET_DISCOVER;
		p->time_sent = clock_time_now();
		p->path[0] = info->myname;

		for (j = 1; j < MAX_NODES; j++)
		{
			p->path[j] = -1;
		}

		net_send((char*)p, sizeof(P),
			 info->myname, info->nip->myneighbors[i]);
	}
}

static int
handle_packet_discover(struct node_info *info, P *p)
{
	P * newp;
	int dest, i, j;

	/*
	 * first, rewrite the packet adding my own name to the end
	 * of the list of nodes seen.
	 */

	for (i = 0; i < MAX_NODES; i++)
	{
		if (p->path[i] == -1)
		{
			p->path[i] = info->myname;
			break;
		}
	}

	if (i == MAX_NODES)
	{
		printf("%d: packet's PATH field is full! Discarding..\n",
		       info->myname);
		return PLEASE_FREE;
	}

	/*
	 * then, compare the list of nodes this packet has seen
	 * to my list of neighbors.  any of my neighbors which has
	 * not seen this packet yet should be sent this.
	 */

	for (i = 0; i < info->nip->neighbors; i++)
	{
		int nbr;

		nbr = info->nip->myneighbors[i];

		/* check if this neighbor has seen the packet */
		for (j = 0; j < MAX_NODES; j++)
		{
			if (p->path[j] == nbr)
				break;
		}

		if (j != MAX_NODES)
		{
			/*
			 * this neighbors of ours has
			 * already seen this packet.
			 */
			continue;
		}

		/* it has not, so send it this packet. */

		newp = NEWP();

		if (newp == NULL)
		{
			printf("out of buffers\n");
			exit(1);
		}

		memcpy(newp, p, sizeof(P));
		newp->destination = nbr;

		net_send((char*)newp, sizeof(P), info->myname, nbr);
	}

	/*
	 * finally, send a PACKET_DISCOVER_RESPONSE backwards to
	 * the originator (but follow the path backwards!)
	 */

	p;

	p->destination = p->source;
	p->source = info->myname;
	p->type = PACKET_DISCOVER_REPLY;

	dest = p->path[0];

	for (i = 1; i < MAX_NODES; i++)
	{
		if (p->path[i] == -1)
		{
			printf("%d: My own name isn't found when "
			       "attempting to return REPLY!\n",
			       info->myname);
			return PLEASE_FREE;
		}

		if (p->path[i] == info->myname)
			break;

		dest = p->path[i];
	}

	net_send((char*)p, sizeof(P), info->myname, dest);

	return DONT_FREE;
}

static int
handle_packet_discover_reply(struct node_info *info, P *p)
{
	int i, t, dest;

	if (p->destination == info->myname)
	{
		/* 
		 * if the intended destination for this packet is ME,
		 * then process the packet and update my optimal route
		 * information.
		 */
		t = clock_time_now() - p->time_sent;

		if (info->path_length[p->source] > t)
		{
			memcpy(info->node_path[p->source], p->path,
			       sizeof(p->path));

			info->path_length[p->source] = t;
		}

		return PLEASE_FREE;
	}

	/*
	 * else, if this packet is not intended for me, pass it on.
	 * walk the list of nodes in the path in the packet,
	 * finding myself in the list.  then send this packet on
	 * to the next node in the list so that it may finally
	 * reach its destination.
	 */

	for (dest = p->path[0], i = 1;
	     i < MAX_NODES;
	     dest = p->path[i], i++)
	{
		if (p->path[i] == -1)
		{
			printf("%d: My own name isn't found when "
			       "attempting to forward REPLY\n",
			       info->myname);
			return PLEASE_FREE;
		}

		if (p->path[i] == info->myname)
			break;
	}

	net_send((char*)p, sizeof(P), info->myname, dest);

	return DONT_FREE;
}
