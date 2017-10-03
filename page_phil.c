
#define PROXY_SERVER "wwwgate0.mot.com"
#define PROXY_PORT   1080
#define RETURN_LOGFILE "/home/knaack/logs/sendpage"
#define DEFAULT_PAGER "1243841"
#define MSG_TRAILER "--Phil Knaack, pknaack1@email.mot.com, 1243841@skytel.com"
#define RESPONSE_ADDR DEFAULT_PAGER "@skytel.com"

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <fcntl.h>
#include <netdb.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <termios.h>

static char * program_name;

struct message_params {
    char pager_number[20];
    char message[512];
    int background;
};

static void   usage( char * );

int    do_connect( char * host, int port );
char * urlify( char * );

static char * parse_cmd_line( int argc, char ** argv,
                              struct message_params * );
static char * build_put_data( struct message_params *, char * data );

int
page_phil_main( int argc, char ** argv )
{
    int fd, sz, logfd;
    FILE * out;
    char * t;
    struct message_params parms;
    char putdata[650];
    char readbuf[512];

    program_name =
        (( t = strrchr( argv[0], '/' )) == NULL ) ?
        argv[0] : (t+1);

    if (( t = parse_cmd_line( argc, argv, &parms )) != NULL )
    {
        usage( t );
        return 1;
    }

    if (( t = build_put_data( &parms, putdata )) != NULL )
    {
        usage( t );
        return 1;
    }

    /* 
     * at this point we've verified most things.  we'll attempt to 
     * make the connection in the background.
     */

    if ( parms.background == 1 )
    {
        int pid;

        pid = fork();

        if ( pid > 0 )
        {
            /*
             * fork successful. we are the parent,
             * and the child is on its way.
             */
            return 0;
        }

        if ( pid < 0 )
        {
            fprintf( stderr,
                     "%s: Could not fork background "
                     "process, will send "
                     "page in foreground.\n", program_name );
        }

        logfd = open( RETURN_LOGFILE,
                      O_WRONLY | O_CREAT | O_APPEND,
                      0644 );

        if ( logfd < 0 )
            logfd = 1;
    }
    else
    {
        printf( "Attempting connection to WWW proxy server\n" );
        logfd = 1;
    }

    fd = do_connect( PROXY_SERVER, PROXY_PORT );

    if ( fd < 0 )
    {
        printf( "%s: Unable to connect to web proxy server!\n",
                program_name );
        return 0;
    }

    if ( parms.background == 0 )
    {
        printf( "Connection established on fd %d\n", fd );
    }

    out = fdopen( fd, "w" );

    fprintf( out,
             "POST http://www.skytel.com/cgi-bin/page.pl HTTP/1.0\n"
             "Accept: image/gif, image/x-xbitmap, image/jpeg, "
             "image/pjpeg, image/png, */*\n"
             "Content-type: application/x-www-form-urlencoded\n"
             "Content-length: %d\n\n", strlen( putdata ));

    fprintf( out, "%s\n", putdata );
    fflush( out );

    if ( parms.background == 0 )
    {
        printf( "POST http://www.skytel.com/cgi-bin/page.pl HTTP/1.0\n" );
        printf( "%s\n", putdata );
    }

    do {
        sz = read( fd, readbuf, 512 );

        if ( sz > 0 )
        {
            write( logfd, readbuf, sz );
        }
    } while ( sz > 0 );

    if ( parms.background == 0 )
    {
        printf( "Closing connection.\n" );
    }

    fclose( out );
    close( fd );

    return 0;
}

char *
parse_cmd_line( int argc, char ** argv, struct message_params * parms )
{
    int i;

    parms->background = 0;

    if ( argc == 1 )
    {
        return "";
    }

    if ( argv[1][0] == '-'  && argv[1][1] == 'b' )
    {
        parms->background = 1;
        argv++;
        argc--;
    }

    if ( argv[1][0] == '-' )
    {
        for ( i = 1; i < strlen( argv[1] ); i++ )
            if ( ! isdigit( argv[1][i] ) )
            {
                return "Pager number must be a number\n";
            }

        strcpy( parms->pager_number, argv[1] + 1 );
        argv++;
        argc--;
    }
    else
    {
        strcpy( parms->pager_number, DEFAULT_PAGER );
    }

    if ( argc == 1 )
    {
        return "Command line parameters wrong\n";
    }

    parms->message[0] = 0;

    while ( argc > 1 )
    {
        if (( strlen( parms->message ) + strlen( argv[1] )) > 500 )
        {
            return "Message too long\n";
        }

        strcat( parms->message, argv[1] );
        strcat( parms->message, " " );

        argv++; argc--;
    }

    strcat( parms->message, MSG_TRAILER );

    return NULL;
}

char *
build_put_data( struct message_params * parms, char * data )
{
    char * dptr = data;

#define PRT(x)  sprintf x; dptr += strlen( dptr )
#define SEP()  *dptr++ = '&'

    PRT(( dptr, "success_url=http://www.cig.mot.com/Organization/GSM/" ));
    SEP();
    PRT(( dptr, "to=%s", parms->pager_number ));
    SEP();
    PRT(( dptr, "pager=1" ));
    SEP();
    PRT(( dptr, "response=%s", urlify( RESPONSE_ADDR )));
    SEP();
    PRT(( dptr, "message=%s", urlify( parms->message )));
    SEP();
    PRT(( dptr, "count=%d", strlen( parms->message )));

    return 0;
}

void
usage( char * before )
{
    if ( before != NULL )
    {
        fprintf( stderr, before );
    }

    fprintf( stderr, "usage: " );
    fprintf( stderr,
             "%s [-b] [-pager number] message...\n",
             program_name );
}
