#include "bbs.h"

int supertid;
struct client *outfrom;
char *superoutbuf;
int superoutsize;

void
super(void *dum)
{
	struct client *c;

	supertid = th_tid();

	printf("Supervisor task running.\n");

	while (!done)
	{
		/* i get resumed when I got somethin to do. */
		th_suspend(0);

		if (outfrom != NULL)
			printf("Supervisor: %d bytes from %s\n", 
			       superoutsize, outfrom->name);
		else
			printf("Supervisor: %d bytes of login info\n", 
			       superoutsize);

		for (c = clients; c != NULL; c = c->next)
		{
			if (c == outfrom)
				continue;

			if (outfrom != NULL)
			{
				ringbuf_add(c->outputq, "[", 1);
				ringbuf_add(c->outputq, outfrom->name,
					    strlen(outfrom->name));
				ringbuf_add(c->outputq, "] ", 2);
			}

			ringbuf_add(c->outputq, superoutbuf, superoutsize);

			if (c->outputting == 0 && c->waiting == 1)
				th_resume(c->tid);

			c->outputting = 1;
		}
	}

	supertid = 0;
}

void
superqueue(struct client *from, char *buf, int size)
{
	if (supertid > 0)
	{
		superoutbuf = buf;
		superoutsize = size;
		outfrom = from;
		th_resume(supertid);
	}
}
