/*
    This file is part of the "pfkutils" tools written by Phil Knaack
    (pknaack1@netscape.net).
    Copyright (C) 2008  Phillip F Knaack

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdlib.h>
#include <sys/time.h>

#include "ipipe_stats.h"

#define TICKS_PER_SECOND 2
#define UINT64  unsigned long long

static UINT64 bytes_read;
static UINT64 bytes_written;

static bool shortform;
static bool verbose;
static bool final;

#define TICKS_IN_5 (TICKS_PER_SECOND * 5)

static            int last5pos;
static struct timeval last5times [ TICKS_IN_5 ];
static         UINT64 last5sr    [ TICKS_IN_5 ];
static         UINT64 last5sw    [ TICKS_IN_5 ];

static int connections;
static bool is_proxy = true;
static struct timeval time_start;
static struct timeval next_print;
static struct timeval interval;

static int
difftimevals( struct timeval * a, struct timeval * b )
{
    struct timeval diff;
    int elapsed100ths;

    // if a < b, return -1;
    if (a->tv_sec < b->tv_sec)
        return -1;
    if (a->tv_sec == b->tv_sec  &&  a->tv_usec < b->tv_usec)
        return -1;

    diff.tv_sec  =  a->tv_sec  - b->tv_sec ;
    diff.tv_usec =  a->tv_usec - b->tv_usec;
    if ( a->tv_usec < b->tv_usec )
    { // borrow!
        diff.tv_usec += 1000000;
        diff.tv_sec  -= 1;
    }
    elapsed100ths = (diff.tv_sec * 100) + (diff.tv_usec / 10000);
    if ( elapsed100ths == 0 )
        elapsed100ths = 1; // prevent divide-by-0 errors
    return elapsed100ths;
}

static void
addtimevals(struct timeval * out, struct timeval * a, struct timeval * b)
{
    out->tv_sec  = a->tv_sec  + b->tv_sec;
    out->tv_usec = a->tv_usec + b->tv_usec;
    if (out->tv_usec > 1000000)
    {
        out->tv_usec -= 1000000;
        out->tv_sec += 1;
    }
}

static void
print_status( bool last )
{
    int rdbps, wrbps, ratio1000 = 1000, ratio10005s = 1000;
    UINT64 rd5s, wr5s;
    int elapsed100ths;
    bool stalled = false;
    static bool printed_header = false;
    struct timeval time_now;

    gettimeofday( &time_now, NULL );
    elapsed100ths = difftimevals( &time_now, &time_start );

    if (( !last && verbose ) ||
        (  last && final   ))
    {
        rdbps = bytes_read    * 100 / elapsed100ths;
        wrbps = bytes_written * 100 / elapsed100ths;

        rd5s = (bytes_read    - last5sr[last5pos]) * 100;
        wr5s = (bytes_written - last5sw[last5pos]) * 100;

        int diff5s = difftimevals( &time_now, &last5times[last5pos] );

        rd5s /= diff5s;
        wr5s /= diff5s;

        if ( rd5s == 0 && wr5s == 0 )
            if ( bytes_read != 0 || bytes_written != 0 )
                stalled = true;

        last5sr    [last5pos] = bytes_read;
        last5sw    [last5pos] = bytes_written;
        last5times [last5pos] = time_now;
        if ( ++last5pos >= TICKS_IN_5 )
            last5pos = 0;

        if ( !shortform && bytes_read != 0 && bytes_written != 0 )
        {
            if ( bytes_read > bytes_written )
                ratio1000 = (bytes_written * 1000) / bytes_read;
            else
                ratio1000 = (bytes_read * 1000) / bytes_written;
            if ( rd5s > wr5s )
                ratio10005s = rd5s > 0 ? ((wr5s * 1000) / rd5s) : 0;
            else
                ratio10005s = wr5s > 0 ? ((rd5s * 1000) / wr5s) : 0;
        }

        if ( !printed_header )
        {
            if ( !shortform )
                fprintf( stderr,
                         "  read (rate, 5s rate)"
                         "  written (rate, 5s rate)"
                         "  ratio (5s ratio) time\n" );
            printed_header = true;
        }

        char outstring[ 160 ], *outs_pos = outstring;
        int slen, outs_remain = sizeof(outstring);
#define ADD_STRING(args...) \
        do { \
            slen = snprintf( outs_pos, outs_remain, args ); \
            outs_remain -= slen; \
            outs_pos += slen; \
        } while(0)

        ADD_STRING("\r " );

        if ( is_proxy )
            ADD_STRING("%d cn%s ",
                       connections, connections==1 ? "" : "s");
        else
            if ( stalled )
                ADD_STRING( "stalled:" );

        if ( shortform )
        {
            ADD_STRING( "%lld bytes in %d.%02ds "
                        "(%d bps)",
                        bytes_written,
                        elapsed100ths / 100,
                        elapsed100ths % 100,
                        wrbps );

            if ( !last )
            {
                ADD_STRING( "(5s avg %d/%d bps)", (int) wr5s, (int)(wr5s*8) );
            }
        }
        else
        {
            if ( last || stalled )
            {
                ADD_STRING( "%lld (%d bps) "
                            "%lld (%d bps) %d.%d%% %d.%02ds",
                            bytes_read,    rdbps,
                            bytes_written, wrbps,
                            ratio1000   / 10,  ratio1000   % 10,
                            elapsed100ths / 100,
                            elapsed100ths % 100 );
            }
            else
            {
                ADD_STRING( "%lld %d/%d "
                            "%lld %d/%d %d.%d%% %d.%d%% %d.%02ds",
                            bytes_read,    rdbps, (int) rd5s,
                            bytes_written, wrbps, (int) wr5s,
                            ratio1000   / 10,  ratio1000   % 10,
                            ratio10005s / 10,  ratio10005s % 10,
                            elapsed100ths / 100,
                            elapsed100ths % 100 );
            }
        }

        slen = strlen(outstring);
        if ( slen < 79 )
        {
            memset( outstring + slen, ' ', 79 - slen );
            outstring[79] = 0;
            fprintf( stderr, "%s", outstring );
        }
        else
        {
            fprintf( stderr, "%s\n", outstring );
        }
    }
}

void
stats_tick( void )
{
    struct timeval t;
    gettimeofday( &t, NULL );
    if (difftimevals( &next_print, &t ) == -1)
    {
        addtimevals( &t, &t, &interval );
        next_print = t;
        print_status(false);
    }
}

void
stats_init ( bool _shortform, bool _verbose,
             bool _final, bool _is_proxy, struct timeval * tick_tv )
{
    int i;

    bytes_read = 0;
    bytes_written = 0;
    connections = 0;

    memset( &last5sr, 0, sizeof( last5sr ));
    memset( &last5sw, 0, sizeof( last5sw ));
    last5pos = 0;

    interval.tv_sec = 0;
    interval.tv_usec = 1000000 / TICKS_PER_SECOND;

    gettimeofday( &time_start, NULL );
    addtimevals( &next_print, &time_start, &interval );

    for ( i = 0; i < TICKS_IN_5; i++ )
        last5times[i] = time_start;

    shortform = _shortform;
    verbose = _verbose;
    final = _final;
    is_proxy = _is_proxy;

    if ( verbose )
        final = true;

    tick_tv->tv_sec = 0;
    tick_tv->tv_usec = 1000000 / TICKS_PER_SECOND / 2;
}

void
stats_add_conn ( void )
{
    connections++;
}

void
stats_del_conn ( void )
{
    connections--;
}

void
stats_reset( void )
{
    gettimeofday( &time_start, NULL );
    addtimevals( &next_print, &time_start, &interval );
}

void
stats_add( int r, int w )
{
    bytes_read    += r;
    bytes_written += w;
    stats_tick();
}

void
stats_done ( void )
{
    print_status(true);
    fprintf( stderr, "\n" );
}
