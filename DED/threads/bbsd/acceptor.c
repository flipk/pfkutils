#include "bbs.h"

int done;
int acceptortid;

void log_new_conn( int, struct sockaddr_in * );

void
acceptor( void *sock0 )
{
	int sock, fds[1], ofds[1], ret, ear;
	struct sockaddr_in sa;

	sock = (int)sock0;
	acceptortid = th_tid();
	clients = NULL;
	fds[0] = sock;
	done = 0;

	printf( "Acceptor task %d running.\n", acceptortid );

	(void) listen( sock, 2 );

	while ( !done )
	{
		ret = th_select( 1, fds, 0, NULL, 1, ofds, -1 );
		if ( ret > 0 )
		{
			int size;
			size = sizeof( struct sockaddr_in );

			ear = accept( sock, (struct sockaddr *) &sa, &size );

			if ( ear > 0 )
			{
				log_new_conn( ear, &sa );
				th_create( client, (void*)ear,
					   PRIO_CLIENT, "client" );
			}
		}
	}
}

void
log_new_conn( int fd, struct sockaddr_in * sa )
{
	unsigned char *addr;

	addr = (char*) &sa->sin_addr.s_addr;

	printf( "Acceptor: new connection fd %d address %d.%d.%d.%d\n",
		fd, addr[0], addr[1], addr[2], addr[3] );
}
