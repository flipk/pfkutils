/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "packet_decoder.H"
#include "packet_encoder.H"
#include "base64.h"

struct etg_stats stats;

static time_t  last_print;

void
print_stats( void )
{
    time_t now = time(NULL);
    if ( last_print == now )
        return;
    last_print = now;
    printf( "\r  tx packets %d bytes %d  rx packets %d bytes %d\r",
            stats.out_packets, stats.out_bytes,
            stats.in_packets,  stats.in_bytes );
    fflush( stdout );
}
