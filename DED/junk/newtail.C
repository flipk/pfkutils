
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/uio.h>
#include <sys/stat.h>

#include "threads.H"

extern "C" {
    int newtail_main( int argc, char ** argv );
}

static int num_tail_threads;

class NewTailThread : public Thread {
    void entry( void );
    void add_char( int c );
    char * fn;
    FILE * f;
    static const int max_line = 80;
    char curline[ max_line ];
    int curline_l;
public:
    NewTailThread( char * _fn );
    ~NewTailThread( void );
};

class MonitorThread : public Thread {
    void entry( void );
    char * fn;
public:
    MonitorThread( char *_fn )
        : Thread( "monitor" ) {
        fn = _fn;
        resume( tid );
    }
};

static bool newtail_die;
#define PIDFILE "%s/.newtail.pid"

int
newtail_main( int argc, char ** argv )
{
    char pidfile[80];

    sprintf( pidfile, PIDFILE, getenv( "HOME" ));

    if ( argc == 2 && strcmp( argv[1], "-q" ) == 0 )
    {
        unlink( pidfile );
        return 0;
    }

    num_tail_threads = 0;

    ThreadParams p;
    p.my_eid = 1;
    Threads th( &p );
    while ( argc > 1 )
    {
        (void) new NewTailThread( argv[1] );
        argc--;
        argv++;
    }

    newtail_die = false;
    (void) new MonitorThread( pidfile );
    th.loop();
    return 0;
}

void
MonitorThread :: entry( void )
{
    struct stat sb;
    FILE * fd;

    (void) unlink( fn );
    fd = fopen( fn, "w" );
    fprintf( fd, "%d", getpid() );
    fclose( fd );

    while ( num_tail_threads > 0 )
    {
        if ( stat( fn, &sb ) < 0 )
            break;
        sleep( tps() * 10 );
    }

    newtail_die = true;
}

NewTailThread :: NewTailThread( char * _fn )
    : Thread( "tail" )
{
    fn = _fn;
    f = NULL;
    curline_l = 0;
    num_tail_threads++;
    resume( tid );
}

NewTailThread :: ~NewTailThread( void )
{
    num_tail_threads--;
}

void
NewTailThread :: entry( void )
{
    while ( !newtail_die )
    {
        struct stat sb1, sb2;

        printf( "%s: opening\n", fn );

        if ( stat( fn, &sb1 ) < 0 )
        {
            printf( "%s: stat error: %s\n",
                    fn, strerror( errno ));
            break;
        }

        f = fopen( fn, "r" );

        if ( f == NULL )
        {
            printf( "%s: unable to fopen: %s\n",
                    fn, strerror( errno ));
            break;
        }

        while ( !newtail_die )
        {
            int c;

            while (( c = getc( f )) != EOF )
                add_char( c );

            if ( ferror( f ))
            {
                printf( "%s: error: %s\n",
                        fn, strerror( errno ));
                break;
            }
            else
                clearerr( f );

            sleep( tps() );

            if ( stat( fn, &sb2 ) < 0 )
            {
                printf( "%s: stat error: %s\n",
                        fn, strerror( errno ));
                break;
            }

            if ( sb1.st_ino != sb2.st_ino )
            {
                printf( "%s: inode # has changed\n" );
                break;
            }
        }

        printf( "%s: closing\n" );
        fclose( f );
    }
}

void
NewTailThread :: add_char( int c )
{
    curline[curline_l++] = (char)c;

    if ( c == '\n' || curline_l == (max_line-1) )
    {
        if ( c != '\n' )
            curline[curline_l++] = '\n';

        iovec vec[3];

        vec[0].iov_base = fn;
        vec[0].iov_len = strlen( fn );
        vec[1].iov_base = (typeof(vec[1].iov_base))": ";
        vec[1].iov_len = 2;
        vec[2].iov_base = curline;
        vec[2].iov_len = curline_l;

        ::writev( 1, vec, 3 );

        curline_l = 0;
    }
}
