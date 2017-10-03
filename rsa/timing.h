
#include <sys/time.h>

#define TS_VAR struct timeval
#define TS_START(v) gettimeofday( &v, 0 )
#define TS_END(v) _ts_endtime( &v )

int _ts_endtime( struct timeval * s );
