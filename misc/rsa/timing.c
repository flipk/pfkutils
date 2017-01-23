
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
