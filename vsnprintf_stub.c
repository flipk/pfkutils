
/*
 * wow, this is really gross.
 * i don't know why but if you build crunched_utils for solaris 5.6
 * and then run the binary on 5.5.1 it doesn't work because it claims
 * 'vsnprintf' is undefined. but i can't figure out where vsnprintf
 * is being referenced from. i suspect libcurses because i can't figure
 * out where the linker is getting libcurses from, and 'he' doesn't work
 * without a properly functioning vsnprintf.
 */

#include <stdarg.h>
#include <stdio.h>

#define ARBITRARY_MAX_BUF 16384

int
vsnprintf( char *str, size_t size, const char *format, va_list ap )
{
    char * temp = (char*)malloc( ARBITRARY_MAX_BUF );
    int ret;

    /* don't depend on return value of vsprintf. */
    /* on sunos it is a char* instead of an int. */
    vsprintf( temp, format, ap );
    ret = strlen( temp );

    if ( ret > size )
        ret = size-1;

    memcpy( str, temp, ret );
    free( temp );
    str[ret] = 0;

    return ret;
}

int
snprintf( char *str, size_t size, const char *format, ... )
{
    int ret;
    va_list ap;

    va_start( ap, format );
    ret = vsnprintf( str, size, format, ap );
    va_end( ap );

    return ret;
}
