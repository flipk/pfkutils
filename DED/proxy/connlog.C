
#include "config.h"

#include <stdarg.h>

#include "connlog.H"

static int last_index = 1;
static int last_print = 1;

connLog :: connLog( void )
{
    char fname[80];

    lognumber = last_index++;
    sprintf( fname, "/home/flipk/log/connlog.%03d", lognumber );

#ifndef ENABLE_LOG
    f = NULL;
#else
    f = fopen( fname, "w" );
#endif
}

connLog :: ~connLog( void )
{
    if ( f )
        fclose( f );
}

void
connLog :: logdat( direction dir, char * buf, int _len )
{
    if ( f == NULL )
        return;

    char * _dir;
    switch ( dir )
    {
    case FROM_BROWSER: _dir = "from browser"; break;
    case TO_BROWSER:   _dir = "to browser";   break;
    case FROM_SITE:    _dir = "from site";    break;
    case TO_SITE:      _dir = "to site";      break;
    }

    fprintf( f, "\ndata direction: %s\n", _dir );

    int len = _len;

    if (( len % 16 ) != 0 )
        len = ((len / 16) + 1) * 16;

    for ( int i = 0; i < len; i++ )
    {
        int col = i % 16;
        unsigned char c = buf[i];
        if ( col == 0 )
            fprintf( f, "\n%04x  ", i );
        if ( i < _len )
            fprintf( f, "%02x ", c );
        else
            fprintf( f, "   " );
        if ( col == 7 )
            fprintf( f, "  " );
        if ( col == 15 )
        {
            fprintf( f, "    " );
            for ( int j = i-15; j <= i && j < _len; j++ )
            {
                unsigned char c = buf[j];
                if ( c >= 0x20 && c <= 0x7f )
                    fprintf( f, "%c", c );
                else
                    fprintf( f, "." );
            }
        }
    }
    fprintf( f, "\n" );

    fflush( f );
}

void
connLog :: print( char * format, ... )
{
    va_list ap;

    if ( f == NULL )
        return;

    fprintf( f, "print %4d: ", last_print++ );
    va_start( ap, format );
    vfprintf( f, format, ap );
    va_end( ap );

    fflush( f );
}
