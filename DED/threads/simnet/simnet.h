#define TRUE 1
#define FALSE 0
#ifndef NULL
#define NULL 0
#endif

#define MAX_NODES 20

extern int simulator_exit;
extern int messages_received;

struct node_identity {
	int myname;
	int neighbors;
	int myneighbors[0];
};

void   topology_launch(void);
int    topology_path_length(int from, int to);

void   clock_init(void);
int    clock_time_now(void);

void   inject_initial_packet(void);
void   node_main(void *myname0);

void   net_init(void);
void   net_destroy(void);
void   net_init_node(int myname);
void   net_destroy_node(int myname);
char * net_receive(int *buflen, int *from, int to);
void   net_send(char *buf, int buflen, int from, int to);
int    net_clock_isr(void);
