
#include <arpa/telnet.h>
#include <stdarg.h>

#include "main.H"
#include "messages.H"
#include "supervisor.H"
#include "client.H"
#include "clients.H"

Client :: Client( int _fd, unsigned long _addr )
    : Thread( "Client", MY_PRIO )
{
    unsigned char * a;
    fd = _fd;
    addr = _addr;
    a = (unsigned char *)&addr;
    inbuflen = 0;
    _inbufleft = 0;
    strcpy( username, "NewUser" );
    stdio();
    printf( "Client: new client on fd %d from host %d.%d.%d.%d\n", 
            fd, a[0], a[1], a[2], a[3] );
    unstdio();
    iter = supervisor->global_msgs->newIterator();
    supervisor->clients->add_client( this );
    resume( tid );
}

Client :: ~Client( void )
{
    supervisor->clients->del_client( this );
    supervisor->global_msgs->deleteIterator( iter );
    stdio();
    printf( "Client for fd %d (user '%s') is exiting\n", fd, username );
    unstdio();
    close( fd );
}

void
Client :: entry( void )
{
//  if ( negotiate_telnet() == false )
//      return;

    print( "\n\n"
           "[Welcome] Welcome to BBS.\n" );

//  if ( negotiate_user() == false )
//      return;

    stdio();
    printf( "Client: user '%s' has logged on.\n", username );
    unstdio();

    print_b( "[NOTICE] %s has logged in\n", username );

    bool local_exit = false;
    while ( global_exit == false && local_exit == false )
    {
        int out_fd;
        int cc;
        cc = select( 1, &fd, 0, NULL, 1, &out_fd, 10 );

        Message * m;
        while (( m = supervisor->
                 global_msgs->nextMessage( iter )) != NULL )
        {
            if ( m->sender() == tid )
                continue;

            (void) write( fd, m->data(), m->size() );
        }

        if ( cc != 1 )
            continue;

        if ( getline() == false )
            // connection dead?
            return;

        local_exit = ! handle_input();
    }

    print_b( "[NOTICE] %s has logged out\n", username );
}

void
Client :: print( char * format, ... )
{
    char obuf[ linelen ];
    va_list ap;
    int len;

    // xxx: vsnprintf doesn't return an int on some platforms
    va_start( ap, format );
    len = vsnprintf( obuf, linelen-1, format, ap );
    va_end( ap );

    obuf[ linelen-1 ] = 0;
    (void) write( fd, obuf, len );
}

void
Client :: print_b( char * format, ... )
{
    char obuf[ linelen + maxname + 2 ];
    va_list ap;
    int len;

    // xxx: vsnprintf doesn't return an int on some platforms
    va_start( ap, format );
    len = vsnprintf( obuf, linelen + maxname + 1, format, ap );
    va_end( ap );

    obuf[ linelen + maxname + 1 ] = 0;
    supervisor->enqueue( obuf, len );
}

bool
Client :: negotiate_telnet( void )
{
    // xxx
    // tell remote end to do local echo 
    // and linemode.
}

bool
Client :: negotiate_user( void )
{
    print( "[Welcome] Username: " );

    while ( 1 )
    {
        if ( getline() == false )
            return false;

        if ( inbuflen > maxname )
        {
            print( "[Welcome] Too long, try again: " );
            continue;
        }

        if ( numargs > 1 )
        {
            print( "[Welcome] One word, try again: " );
            continue;
        }

        print( "[Welcome] OK, your username is %s\n", args[0] );
        break;
    }

    inbuf[ inbuflen ] = 0;
    strcpy( username, args[0] );

    return true;
}

// return the next character from the
// input buffer. if none is available, go to the line
// for more data.

int
Client :: _getc( void )
{
    if ( _inbufleft == 0 )
    {
        _inbuflen = read( fd, _inbuf, bufsize );
        if ( _inbuflen <= 0 )
        {
            stdio();
            printf( "read returned %d (%s)\n",
                    _inbuflen, strerror( errno ));
            unstdio();
            return -1;
        }
        _inbufpos = 0;
        _inbufleft = _inbuflen;
    }
    _inbufleft--;
    return _inbuf[ _inbufpos++ ];
}

// do the same as the above, but also process telnet commands.

int
Client :: getc( bool return_telnet )
{
    int c = _getc();
    if ( c == -1 )
        return -1;

    // handle telnet commands here

    return c;
}

static bool
whitespace( int c )
{
    return ( c == 32 || c == 9 || c == 0 );
}

bool
Client :: getline( void )
{
    for ( inbuflen = 0; inbuflen < linelen; )
    {
        int c = getc( false );
        if ( c == -1 )
            return false;
        if ( c == 10 )
            continue;
        if ( c == 13 )
            break;
        inbuf[ inbuflen++ ] = c;
    }

    inbuf[ inbuflen ] = 0;
    memcpy( inbuf2, inbuf, inbuflen+1 );

    int a = 0;
    int i = 0;

    for ( a = 0; a < maxargs; a++ )
    {
        int startoffset = i;
        args[a] = inbuf2 + i;
        while ( ! whitespace( inbuf2[i] ))
            i++;
        inbuf2[i] = 0;
        arglen[a] = i - startoffset;
        while ( whitespace( inbuf2[i] ))
            i++;
        if ( i >= inbuflen )
            break;
    }

    numargs = a+1;
    return true;
}

bool
Client :: handle_input( void )
{
    static struct command_table {
        char * name;
        bool (Client::*func)( void );
        char * helpinfo;
    } cmd_tbl[] = { 
        "help" ,  NULL,
        "Display this help text" ,
        "quit" , &Client::command_quit,
        "Quit your BBS session"  ,
        "emote", &Client::command_emote,
        "Display emoticons, i.e. '/emote is curious.'",
        "b"    , &Client::command_b,
        "Broadcast a message; '/b <message>' or '/b' by itself",
        "who"  , &Client::command_who,
        "Display list of users currently online"  ,
        "kill",  &Client::command_kill,
        "Shutdown the BBS server (requires password)",
        "nick",  &Client::command_nick,
        "Change username; argument is new nick",
        0, 0, 0
    };

    if ( inbuf[0] != '/' )
    {
        print( "[NOTICE] Enter '/help' for help\n" );
        return true;
    }

    struct command_table * ctp;

    for ( ctp = cmd_tbl; ctp->name != NULL; ctp++ )
        if ( strcmp( args[0]+1, ctp->name ) == 0 )
            break;

    if ( ctp->name == NULL )
    {
        print( "[NOTICE] Unknown command %s, type /help\n", args[0] );
        return true;
    }

    if ( ctp->func == NULL )
    {
        // special case for help, because we can't export
        // command table to help function
        print( "[NOTICE] Commands:\n" );
        for ( ctp = cmd_tbl; ctp->name != NULL; ctp++ )
            print( "[NOTICE]   %10s : %s\n",
                   ctp->name, ctp->helpinfo );
        print( "[NOTICE] End of help output\n" );
        return true;
    }

    return (this->*(ctp->func))();
}

bool
Client :: command_quit( void )
{
    print( "[NOTICE] Goodbye! \n" );
    return false;
}

bool
Client :: command_emote( void )
{
    print_b ( "[NOTICE] %s %s\n", username, line_remainder( 1 ));
    print   ( "[NOTICE] %s %s\n", username, line_remainder( 1 ));
    return true;
}

bool
Client :: command_b( void )
{
    if ( numargs > 1 )
    {
        print   ( "[%s] %s\n", username, line_remainder( 1 ));
        print_b ( "[%s] %s\n", username, line_remainder( 1 ));
        return true;
    }

    print( "[NOTICE] Enter message; finish with empty line, "
           "or 'ABORT' by itself.\n" );

    static const int maxlines = 100;
    char * lines [ maxlines ];
    int i;
    bool send = false;
    bool ret = true;

    for ( i = 0; i < maxlines; i++ )
        lines[i] = NULL;

    i = 0;

    while ( i < maxlines )
    {
        print( "[%s] ", username );
        if ( getline() == false )
        {
            ret = false;
            break;
        }
        if ( inbuflen == 0 )
        {
            send = true;
            break;
        }
        if ( strcmp( inbuf, "ABORT" ) == 0 )
            break;

        lines[i] = new char[ inbuflen+1 ];
        memcpy( lines[i], inbuf, inbuflen+1 );
        i++;
    }

    i = 0;

    if ( ret == true && send == true )
    {
        while ( lines[i] != NULL )
        {
            print_b( "[%s] %s\n", username, lines[i] );
            i++;
        }
        print( "[NOTICE] Your message has been sent.\n" );
    }

    if ( send == false && ret == true )
    {
        // it was aborted. tell the user it is aborted.
        print( "[NOTICE] Your message was aborted.\n" );
    }

    for ( i = 0; i < maxlines; i++ )
        if ( lines[i] != NULL )
            delete[] lines[i];

    return ret;
}

bool
Client :: command_who( void )
{
    char ** usernames = supervisor->clients->get_usernames();
    int i = 0;
    int j = 0;

    print( "[NOTICE] The following users are online:\n" );
    while ( usernames[i] != NULL )
    {
        if ( j == 0 )
            print( "[NOTICE]   ---> " );
        print( "%s   ", usernames[i++] );
        if ( ++j == 4 )
        {
            print( "\n" );
            j = 0;
        }
    }

    if ( j != 0 )
        print( "\n" );

    print( "[NOTICE] End of userlist.\n" );

    delete[] usernames;
    return true;
}

bool
Client :: command_kill( void )
{
    print( "[NOTICE] This command requires a password!\n" );
    print_b( "[WARNING] User %s has entered the 'kill' command.\n",
             username );
    print( "[NOTICE] Please enter the kill password: " );
    if ( getline() == false )
    {
        print_b( "[WARNING] User %s has bailed out while "
                 "running kill command\n", username );
        return false;
    }

    if ( strcmp( inbuf, "killItPlease" ) == 0 )
    {
        print( "[NOTICE] That is the correct password. "
               "The server is on the way down.\n" );
        print_b( "[WARNING] User %s knows "
                 "the kill password!\n", username );
        print_b( "[WARNING] This server is being shut down.\n" );
        sleep( TimerThread::tps * 4 );
        global_exit = true;
        return true;
    }

    print_b( "[WARNING] User %s does not know the kill password.\n",
             username );

    print( "[NOTICE] The password given is incorrect.\n" );

    return true;
}

bool
Client :: command_nick( void )
{
    if ( numargs != 2 )
    {
        print( "[NOTICE] /nick usage:   /nick <newusername>\n" );
        return true;
    }

    if ( arglen[1] > maxname )
    {
        print( "[NOTICE] new nick is too long (max %d)\n", maxname );
        return true;
    }

    print( "[NOTICE] ok, your new nick is %s\n", args[1] );
    print_b( "[NOTICE] User %s has changed username to %s\n",
             username, args[1] );

    stdio();
    printf( "Client: user '%s' has changed username to '%s'\n",
            username, args[1] );
    unstdio();

    strcpy( username, args[1] );
    return true;
}
