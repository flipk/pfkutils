#include <stdio.h>
#include <sys/time.h>
#include "th.h"

#define CMD_END 1
#define CMD_DATA 2
#define CMD_ACK 3

struct data {
	int cmd;
	int data[1];
};

#define QUEUE1 2700
#define QUEUE2 2900

static mpoolset * msg_pools;

static void sender( void * );
static void receiver( void * );

int
msgq2_test( void )
{
	static int quantities[] = { 2, 20, 20 };
	static int sizes[] = { MSGQ_SIZE, MSG_STRUCT_SIZE, 64 };
	static int numpools = 3;

	th_init();
	msg_pools = poolset_init( numpools, quantities, sizes );
	th_create( receiver, 0, 5, "recv" );
	th_create( sender,   0, 6, "send" );
	th_loop();

	return 0;
}

static void
sender( void * dummy )
{
	char dat[32];
	int cc, i;
	struct data *d;
	struct data timermsg;

	printf( "The sender is starting.\n" );

	msg_register( QUEUE2, msg_pools );

	th_suspend( 0 );

	d = (struct data *)dat;
	timermsg.cmd = CMD_END;
	timer_create( 5000, QUEUE2, (void*)&timermsg, 4, NULL );

	while (1)
	{
		for (i=0; i < 10; i++)
		{
			d->cmd = CMD_DATA;
			msg_send( QUEUE1, (void*)d, 32 );
		}
		msg_recv( QUEUE2, (void*)d, 32, 0 );
		if ( d->cmd == CMD_END )
		{
			msg_send( QUEUE1, (void*)d, 4 );
			break;
		}
	}

	msg_deregister( QUEUE2 );

	printf( "sender is done.\n" );
}

static void
receiver( void * dummy )
{
	char dat[32];
	int cc, i;
	struct data *d;
	struct timeval tp1, tp2;
	struct timezone tzp;

	printf( "the receiver is starting.\n" );

	msg_register( QUEUE1, msg_pools );

	d = (struct data *)dat;

	gettimeofday( &tp1, &tzp );
	i = 0;
	while (1)
	{
		cc = msg_recv( QUEUE1, dat, 32, 0 );
		if ( d->cmd == CMD_END )
			break;
		if (( ++i % 10 ) == 0 )
		{
			d->cmd == CMD_ACK;
			msg_send( QUEUE2, dat, 4 );
		}
	}
	gettimeofday( &tp2, &tzp );

	msg_deregister( QUEUE1 );

	printf( "receiver is done. got %d msgs\n", i );
	printf( "%d.%06d  to %d.%06d\n", 
		tp1.tv_sec, tp1.tv_usec,
		tp2.tv_sec, tp2.tv_usec );
}
