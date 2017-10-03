/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

#include "timing.h"

int
_ts_endtime( struct timeval * s )
{
    struct timeval e, d;

    gettimeofday( &e, 0 );

    d.tv_sec  = e.tv_sec  - s->tv_sec;
    d.tv_usec = e.tv_usec - s->tv_usec;

    if ( d.tv_usec < 0 )
    {
        d.tv_usec += 1000000;
        d.tv_sec -= 1;
    }

    return d.tv_usec / 1000 + d.tv_sec * 1000;
}
