
/*
 * this file implements a shortest-path algorithm.
 * 
 * each node performs the same algorithm: when a packet is received,
 * that packet contains a list of nodes it has visited thus far on 
 * its journey.  compare that list to the list of immediate neighbors
 * each node knows about.  send the packet on to every neighbor which
 * has not yet seen the node.  also while sending, add your own name
 * to the list.  a packet finally dies if all neighbors have seen it.
 *
 * at startup there is no activity.  but then the initial packet is 
 * injected which causes a flurry of activity.
 * 
 * the very first packet to reach the destination contains the shortest
 * possible path from start to finish contained in it.  the last packet
 * to arrive at the destination contains in it the longest possible
 * packet from the source.
 */

#include "th.h"
#include "simnet.h"

void
inject_initial_packet()
{
	char *buf;

	buf = (char*) malloc(2);
	strcpy(buf, "");
	net_send(buf, 2, 0, 0);
}

static void resend_packet(struct node_identity *nip,
			  char *buf, int len, int myname);

void
node_main(void *myname0)
{
	struct node_identity *nip;
	char *buf;
	int i, myname;

	nip = (struct node_identity *)myname0;
	myname = nip->myname;

	net_init_node(myname);

	printf("%d: I am alive. My neighbors are: ", myname);
	for (i=0; i < nip->neighbors; i++)
		printf("%d ", nip->myneighbors[i]);
	printf("\n");

	while (simulator_exit == FALSE)
	{
		int len, from;
		buf = net_receive(&len, &from, myname);
		if (buf == NULL)
			continue;
		printf("%d: packet from %d: %s\n", myname, from, buf);
		resend_packet(nip, buf, len, myname);
		free(buf);
	}

	printf("%d: exiting\n", myname);

	net_destroy_node(myname);
}

static int
get_next_id(char ** cp)
{
	int ret;

	if (**cp == 0)
		return -1;

	while (!isdigit(**cp))
	{
		if (**cp == 0)
			return -1;
		else
			(*cp)++;
	}

	ret = atoi(*cp);

	while (isdigit(**cp))
		(*cp)++;

	return ret;
}

static void
resend_packet(struct node_identity *nip,
	      char *buf,
	      int len,
	      int myname)
{
	char tmp[10], *cp;
	char *newbuf;
	int newlen, to, count;
	int mydestinations[MAX_NODES];
	int destinations;
	int sent;

	sprintf(tmp, " %d", myname);

	cp = buf;
	destinations = 0;

	for (count=0; count < nip->neighbors; count++)
		mydestinations[destinations++] = nip->myneighbors[count];

	while ((to = get_next_id(&cp)) != -1)
	{
		for (count=0; count < destinations; count++)
			if (mydestinations[count] == to)
			{
				mydestinations[count] = -1;
			}
	}

	sent = 0;

	for (count=0; count < destinations; count++)
	{
		to = mydestinations[count];
		if (to == -1)
			continue;

		newlen = len + strlen(tmp);
		newbuf = (char*) malloc(newlen);

		strcpy(newbuf, buf);
		strcat(newbuf, tmp);

		if (sent++ == 0)
			printf("%d: sending packet on to ", myname);

		printf("%d ", to);

		net_send(newbuf, newlen, myname, to);

	}

	if (sent == 0)
		printf("%d: [packet dies]\n", myname);
	else
		printf("\n");
}
