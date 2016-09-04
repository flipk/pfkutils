/*
 * priority of various tasks within this server
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>

#include "th.h"

#define PRIO_ACCEPTOR 4
#define PRIO_CLIENT   5
#define PRIO_SUPER    6

#define MAX_CLIENTS  32
#define MAX_MSGS     16384

extern int done;
extern int acceptortid;

struct client {
	FILE *out;
	FILE *in;
	char name[64];
	int fd;
	int tid;
	int waiting;
	int outputting;
	ringbuf *outputq;
	struct client *next, *prev;
};

extern struct client *clients;

void  acceptor   (void *);
void  client_init(void);
void  client     (void *);
void  super      (void *);
void  superqueue (struct client *, char *dat, int size);
int   process_command(struct client *, char *);
