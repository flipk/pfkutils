#if 0
ccsimso -I /usr/test/bssdata2/tornado3/vxsim20.0/target/h -DCPU=SIMSPARCSOLARIS -c buffered_printf.c
exit 0
#endif

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <fioLib.h>

#define BUFFER_SIZE 512

static char print_buf[ BUFFER_SIZE+1 ];
static int  print_buf_pos = 0;

#define LEFT() (BUFFER_SIZE - print_buf_pos)

void
flush_buffered_printf( void )
{
    if ( print_buf_pos > 0 )
    {
        /* note that print_buf is 1 larger than BUFFER_SIZE
           so it is legal to do this write even if 
           print_buf_pos==BUFFER_SIZE */
        print_buf[print_buf_pos] = 0;
        lprintf( EXEC_MON_TERM_ID, "%s", print_buf );
    }
    print_buf_pos = 0;
}

static int
printout( char * buffer, int nchars, int outarg )
{
    while ( nchars > 0 )
    {
        int cpy = nchars;
        if ( cpy > LEFT() )
            cpy = LEFT();

        memcpy( print_buf + print_buf_pos, buffer, cpy );

        print_buf_pos += cpy;
        if ( print_buf_pos == BUFFER_SIZE )
            flush_buffered_printf();

        buffer += cpy;
        nchars -= cpy;
    }
    return OK;
}

void
buffered_printf( char * format, ... )
{
    va_list ap;

    va_start( ap, format );
    fioFormatV( format, ap, printout, 0 );
    va_end( ap );
}
