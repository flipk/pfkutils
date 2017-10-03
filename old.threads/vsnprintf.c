
/*
 * wow, this is really gross.
 * i don't know why but if you build crunched_utils for solaris 5.6
 * and then run the binary on 5.5.1 it doesn't work because it claims
 * 'vsnprintf' is undefined. but i can't figure out where vsnprintf
 * is being referenced from. i suspect libcurses because i can't figure
 * out where the linker is getting libcurses from, and 'he' doesn't work
 * without a properly functioning vsnprintf.
 * bunch of other gross shit to make this work on sunos
 * where there is no stdarg, and varargs instead.
 */

#define ARBITRARY_MAX_BUF 16384

#if defined(SUNOS)
#undef __GNUC__
#include <varargs.h>
#include <stdio.h>
#else
#include <stdarg.h>
#include <stdio.h>
#endif


#if defined(SUNOS)
int
vsnprintf( char *str, int size, char *format, va_list ap )
#else
int
vsnprintf( char *str, size_t size, const char *format, va_list ap )
#endif
{
    char * temp = (char*)malloc( ARBITRARY_MAX_BUF );
    int ret;

    /* don't depend on return value of vsprintf. */
    /* on sunos it is a char* instead of an int. */
    vsprintf( temp, format, ap );
    ret = strlen( temp );

    if ( ret > ( size-1 ))
        ret = size-1;

    memcpy( str, temp, ret );
    free( temp );
    str[ret] = 0;

    return ret;
}

#if defined(SUNOS)
int
snprintf( str, size, format, va_alist )
    char *str;
    size_t size;
    char *format;
    va_dcl
#else
int
my_snprintf( char *str, size_t size, char *format, ... )
#endif
{
    int ret;
    va_list ap;

#if defined(SUNOS)
    va_start( ap );
#else
    va_start( ap, format );
#endif
    ret = vsnprintf( str, size, format, ap );
    va_end( ap );

    return ret;
}
