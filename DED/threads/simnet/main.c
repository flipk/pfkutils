#include <stdio.h>

#include "th.h"
#include "simnet.h"

int messages_received;
int simulator_exit;

int
main(argc, argv)
	int argc;
	char ** argv;
{
	setlinebuf(stdout);

	simulator_exit = FALSE;
	messages_received = 0;

	th_init();
	net_init();
	clock_init();
	topology_launch();
	inject_initial_packet();

	th_loop();

	net_destroy();

	printf("main: total messages received: %d\n", messages_received);
	return 0;
}
