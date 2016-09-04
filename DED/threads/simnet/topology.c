#include "th.h"
#include "simnet.h"

#define MAP2
#include "map.h"

#if NODES > MAX_NODES
#error Please increase MAX_NODES in simnet.h or decrease NODES here.
#endif

void
topology_launch(void)
{
	int i,j;

	/*
	 * launch one thread for each node. 
	 * tell that thread which node it is
	 * and who its neighbors are.
	 */

	for (i=0; i < NODES; i++)
	{
		struct node_identity *nip;
		int neighbors[NODES];
		int count;
		char name[10];

		count=0;

		for (j=0; j < NODES; j++)
		{
			if ((topology_map[i][j] == 0) ||
			    (topology_map[i][j] == -1))
				continue;
			neighbors[count++] = j;
		}

		nip = (struct node_identity *)
			malloc(sizeof(struct node_identity) +
			       (sizeof(int) * count));

		nip->myname = i;
		nip->neighbors = count;
		memcpy(nip->myneighbors, neighbors, sizeof(int) * count);

		sprintf(name, "node %d", i);
		th_create(node_main, (void*)nip, 5, name);
	}
}

int
topology_path_length(int from, int to)
{
	return topology_map[from][to];
}
