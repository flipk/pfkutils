#include "bbs.h"

#define MAX_TOKENS 10

int do_help( struct client *, char ** );
int do_who( struct client *, char ** );
int do_exit( struct client *, char ** );
int do_die( struct client *, char ** );
int do_recall( struct client *, char ** );
int do_unknown( struct client *, char ** );
int do_message_short( struct client *, char ** );
int do_message_long( struct client *, char ** );
void _do_message(struct client *, int);

__inline static void
nuke_newlines( char * buf )
{
	char *c;

	for (c=buf; *c != 0; c++)
	{
		if (*c == 10 || *c == 13)
		{
			*c = 0;
			return;
		}
	}
}

__inline static void
lowercase( char * buf )
{
	while (*buf != 0)
	{
		*buf = tolower(*buf);
		buf++;
	}
}

__inline static int
tokenize( char * buf, char ** tokens )
{
	int numtokens = 0;

	while ((*buf != 0) && (numtokens < MAX_TOKENS))
	{
		tokens[numtokens++] = buf;

		/* skip over token */
		while ((*buf != 0) && (*buf != 32) && (*buf != 9))
		{
			buf++;
		}

		/* skip over whitespace if there is some */
		while (*buf == 32 || *buf == 9)
		{
			*buf++ = 0;
		}
	}

	return numtokens;
}

struct commands {
	char *command;
	int (*func)( struct client * client, char ** tokens );
	char *help;
} commands[] = { 
	{ "help",    do_help,           "This message." },
	{ "who",     do_who,            "Show list of users logged on." },
	{ "m",       do_message_short,  "Enter a message to be broadcast." },
	{ "msg",     do_message_long,   "Enter a multi-line message." },
	{ "exit",    do_exit,           "Exit the BBS system." },
	{ "die",     do_die,            "Kill the BBS server." },
	{ "recall",  do_recall,         "Recall last 40 lines of output." },
	{ NULL,      do_unknown,        "Unknown Command." }
};

int
process_command( struct client * client, char * buf)
{
	char *tokens[MAX_TOKENS];
	int numtokens;
	struct commands * cmd;

	nuke_newlines(buf);
	buf++;
	numtokens = tokenize(buf, tokens);

	if (numtokens == 0)
	{
		fprintf(client->out, "Null command ignored.\n");
		return 0;
	}

	lowercase(tokens[0]);

	for ( cmd = &commands[0]; cmd->command != NULL; cmd++ )
		if ( strcmp( cmd->command, tokens[0] ) == 0 )
			break;

	return cmd->func( client, tokens );

	return 0;
}

int
do_help( struct client * client, char ** tokens )
{
	struct commands * cmd;

	printf("Client %d: User %s requested HELP.\n",
	       client->tid, client->name);

	fprintf(client->out, 
		"\n"
		"     BBS Server commands: \n\n");

	for ( cmd = &commands[0]; cmd->command != NULL; cmd++ )
	{
		fprintf( client->out, "%s:\t\t%s\n",
			 cmd->command, cmd->help );
	}

	fprintf(client->out, "\n");
}

int
do_who( struct client * client, char ** tokens )
{
	struct client *c;

	fprintf(client->out, "    Users logged on:\n");

	for (c = clients; c != NULL; c = c->next)
	{
		fprintf(client->out, "%s\t", c->name);
	}

	fprintf(client->out, "\n\n");

	return 0;
}

int
do_exit( struct client * client, char ** tokens )
{
	return -1;
}

int
do_die( struct client * client, char ** tokens )
{
	printf("Client %d: got DIE command!\n", client->tid);
	done = 1;
	th_resume(acceptortid);
	return 1;
}

int
do_unknown( struct client * client, char ** tokens )
{
	fprintf(client->out, "Unknown command ignored.\n");
	return 0;
}

int
do_recall( struct client * client, char ** tokens )
{
	return 0;
}

int
do_message_long( client, dummy )
	struct client * client;
	char ** dummy;
{
	_do_message( client, 1 );

	return 0;
}

int
do_message_short( client, dummy )
	struct client * client;
	char ** dummy;
{
	_do_message( client, 0 );
	return 0;
}

void
_do_message(client, flag)
	struct client *client;
	int flag;
{
	char buf[2048];
	int cc, cc2;

	if (flag == 0)
	{
		fprintf(client->out,
			"Enter a one-line message.\n[%s] ",
			client->name);
		
	} else {
		fprintf(client->out,
			"Enter a multiline message. Terminate with "
			"a slash on a line by itself.\n");
	}

	fflush(client->out);

	if (flag == 0)
	{
		cc = th_read(client->fd, buf, 128);

		if (cc <= 0)
			return;
	} else {
		cc = 0;
		while (cc < 2048)
		{
			int oldpos;

			if (cc != 0)
			{
				oldpos = cc;
				buf[cc++] = '[';
				strcpy(buf + cc, client->name);
				cc += strlen(client->name);
				buf[cc++] = ']';
				buf[cc++] = ' ';
			}

			cc2 = th_read(client->fd, buf + cc, 2048 - cc);
			
			if (buf[cc] == '/' && 
			    ((buf[cc+1] == 10) || (buf[cc+1] == 13)))
			{
				cc = oldpos;
				break;
			}

			cc += cc2;
		}
	}

	superqueue(client, buf, cc);

	fprintf(client->out, "Message sent.\n");
}
