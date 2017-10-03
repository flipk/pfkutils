
#define MALLOC_LOCK_VERBOSE 0

//
// this file implements a semaphore lock for use with the
// malloc.c function calls to ensure that multiple threads can
// use malloc without worry of contention.
//

#include "../threads/h/threads.H"

static ThreadSemaphore * malloc_sem = NULL;
static int malloc_nesting = 0;

extern "C" void malloclock( int lu );

#if MALLOC_LOCK_VERBOSE
void
format_hex( int h, char * s )
{
    int i;
    char cvt[] = "0123456789abcdef";
    s += 7;
    for ( i = 0; i < 8; i++ )
    {
        *s-- = cvt[ h & 15 ];
        h >>= 4;
    }
}

#define WR(s) ::write( 1, s, strlen( s ))
#endif

void
malloclock( int lu )
{
#if MALLOC_LOCK_VERBOSE
    char str[20];
    format_hex( malloc_nesting, str );
    str[8] = ' ';
    format_hex( (int)malloc_sem, str + 9 );
    str[17] = '\n';
    str[18] = 0;
    WR(str);
#endif

    if ( !th || !th->init_done )
        return;

    if ( malloc_nesting == 0 && malloc_sem == NULL )
    {
        malloc_nesting++;
        malloc_sem = th->sems->seminit( "malloc", 1 );
        malloc_nesting--;
    }

    switch ( lu )
    {
    case 1:
        malloc_nesting++;
        if ( malloc_nesting == 1 )
            malloc_sem->take();
        break;
    case 0:
        malloc_nesting--;
        if ( malloc_nesting == 0 )
            malloc_sem->give();
        break;
    }
}

void
malloclock_die( void )
{
    malloc_nesting = 99;
    if ( th && th->sems && malloc_sem )
    {
        th->sems->semdelete( malloc_sem );
    }
    malloc_sem = NULL;
}

void * operator new     ( size_t s ) { return (void*)malloc( s ); }
void * operator new[]   ( size_t s ) { return (void*)malloc( s ); }
void   operator delete  ( void * p ) { free( p ); }
void   operator delete[]( void * p ) { free( p ); }


void * operator new     ( size_t s, char * file, int line )
{
    return (void*)malloc_record( file, line, s );
}

void * operator new[]   ( size_t s, char * file, int line )
{
    return (void*)malloc_record( file, line, s );
}
