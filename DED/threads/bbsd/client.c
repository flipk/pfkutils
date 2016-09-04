#include "bbs.h"

mpool *clientpool;
struct client *clients = NULL;

#define DISP(fd, x)  write(fd, x, sizeof(x))

void
client_init( void )
{
	clientpool = pool_init( sizeof(struct client),
				MAX_CLIENTS );
}

void
client( void *dat )
{
	struct client *c;
	FILE *out;
	int fd,i,j, cc, writ, size, fds[1], outfds[1], clientdone;
	void *pools;
	char buf[128];

	fd = (int)dat;

	printf( "Client %d running, connected to fd %d\n", th_tid(), fd );

	c = pool_alloc( clientpool );
	if ( c == NULL )
	{
		printf( "Client %d: cannot alloc client struct\n", th_tid() );
		DISP( fd, "cannot alloc client struct\r\n" );
		DISP( fd, "too many users logged on?\r\n" );
		close( fd );
		return;
	}

	c->fd = fd;
	c->tid = th_tid();
	c->out = fdopen( c->fd, "w" );
	c->in  = fdopen( c->fd, "r" );
	c->waiting = 0;
	c->outputting = 0;
	c->outputq = ringbuf_create( MAX_MSGS );

	fds[0] = c->fd;

	fprintf( c->out, "\n\nLogin: " );
	fflush( c->out );
	cc = th_select( 1, fds, 0, NULL, 1, outfds, -1 );

	if ( fscanf( c->in, "%s", c->name ) != 1 )
	{
		close( c->fd );
		printf( "Client %d: connection lost during login\n", c->tid );
		goto freeup;
	}

	sprintf( buf, "User [%s] logged in.\n", c->name );
	size = strlen( buf );

	superqueue( NULL, buf, size );

	printf( "Client %d: user %s has logged on.\n", c->tid, c->name );

	fprintf( c->out,
		 "\nCommand mode ready. \n"
		 "Commands begin with \"/\".  Type \"/help\" for commands. \n"
		 "\n" );

	c->prev = NULL;
	c->next = clients;
	clients = c;
	if ( c->next != NULL )
		c->next->prev = c;

	clientdone = 0;
	while ( !done && !clientdone )
	{
		fflush( c->out );

		if ( c->outputting == 1 )
		{
			writ = 1;
		}
		else
		{
			writ = 0;
		}

		c->waiting = 1;
		cc = th_select( 1, fds, writ, fds, 1, outfds, -1 );
		c->waiting = 0;

		if ( cc == 0 )
			continue;

		if (( outfds[0] & 0x10000 ) != 0 )
		{
			if ( ringbuf_empty( c->outputq ))
			{
				c->outputting = 0;
			}
			else
			{
				cc = ringbuf_remove( c->outputq, buf, 128 );
				fwrite( buf, cc, 1, c->out );
			}
			continue;
		}

		size = read( c->fd, buf, 127 );

		if ( size <= 0 )
			break;

		if ( buf[0] == '/' )
		{
			buf[size] = 0;
			size = process_command( c, buf );
		}

		if ( size > 0 )
			superqueue( c, buf, size );

		if ( size < 0 )
			clientdone = 1;
	}

	printf( "Client %d: %s is logging off.\n", c->tid, c->name );

	sprintf( buf, "User [%s] logged out.\n", c->name );
	size = strlen(buf);

	if (done != 0)
	{
		fprintf( c->out, "\nThe BBS server has been shut down.\n" );
	}

	fprintf( c->out, "Thank you for using the bbs server.\r\n\r\n" );
	fflush( c->out );

	if ( c->prev != NULL )
		c->prev->next = c->next;
	else
		clients = c->next;

	if ( c->next != NULL )
		c->next->prev = c->prev;

	superqueue( NULL, buf, size );

  freeup:
	fclose( c->in );
	fclose( c->out );
	close( c->fd );
	ringbuf_destroy( c->outputq );
	pool_free( clientpool, (void*)c );
}
