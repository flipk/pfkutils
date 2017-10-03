/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

#include <sys/time.h>

#define TS_VAR struct timeval
#define TS_START(v) gettimeofday( &v, 0 )
#define TS_END(v) _ts_endtime( &v )

int _ts_endtime( struct timeval * s );
