#include "bbs.h"

int
main(argc, argv)
	int argc;
	char ** argv;
{
	int port;
	int s, siz;
	struct sockaddr_in sa;

	if (argc != 2)
	{
	  usage:
		printf("usage: bbsd <port>\n");
		return 1;
	}

	port = atoi(argv[1]);
	if (port < 1024 || port > 32767)
		goto usage;

	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0)
	{
		perror("creating socket");
		return 1;
	}

	siz = 64 * 1024;
	(void)setsockopt(s, SOL_SOCKET, SO_RCVBUF, &siz, sizeof(siz));
	(void)setsockopt(s, SOL_SOCKET, SO_SNDBUF, &siz, sizeof(siz));
	siz = 1;
	(void)setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &siz, sizeof(siz));

	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	sa.sin_addr.s_addr = INADDR_ANY;

	if (bind(s, (struct sockaddr *)&sa, sizeof(struct sockaddr)) < 0)
	{
		perror("binding socket to address");
		return 1;
	}

	client_init();

	th_init();
	th_create(super, (void*)NULL, PRIO_SUPER,    "super");
	th_create(acceptor, (void*)s, PRIO_ACCEPTOR, "acceptor");
	th_loop();

	return 0;
}
