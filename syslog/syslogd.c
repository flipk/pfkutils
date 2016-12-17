/*
    This file is part of the "pfkutils" tools written by Phil Knaack
    (pfk@pfk.org).
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

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/uio.h>
#include <string.h>
#include <fcntl.h>

#include "regex.h"
#include "rules.h"
#include "strip_chars.h"
#include "config_file.h"
#include "process_line.h"
#include "regex.h"

#ifdef __sun__
/* i see this in time.h but i didn't investigate why
   this particular prototype doesn't get in scope */
extern struct tm *localtime_r(const time_t *, struct tm *);
#endif

int
syslogd_main( int argc, char ** argv )
{
    int fd, len;
    char buf[ MAX_LINE_LEN + 40 ];
    char timestr[40];
    struct sockaddr_in sa;
    unsigned char * srcaddr = (unsigned char *) &sa.sin_addr.s_addr;
    struct timeval tv;
    struct tm tm_time;
    time_t now, last;

    rule_files = rule_files_tail = NULL;
    rules = rules_tail = NULL;

    if (argc != 1)
    {
        if (argc != 3 || strcmp(argv[1],"-f")!=0)
        {
            fprintf(stderr, "usage:\n"
                    "   syslogd\n"
                    "   syslogd -f syslogd.ini\n");
            exit(1);
        }
        parse_config_file(argv[2]);
    }
    else
        parse_config_file(DEFAULT_CONFIG_FILE);

    fd = socket( PF_INET, SOCK_DGRAM, 0 );
    if ( fd < 0 )
    {
        fprintf( stderr, "unable to open socket: %s\n", strerror( errno ));
        return 1;
    }

    sa.sin_family = AF_INET;
    sa.sin_port = htons( syslogd_port_number );
    sa.sin_addr.s_addr = htonl( INADDR_ANY );

    fprintf(stderr, "using %s UDP port number %d\n",
            (syslogd_port_number==514) ? "standard" : "nonstandard",
            syslogd_port_number);

    if ( bind( fd, (struct sockaddr *)&sa, sizeof( sa )) < 0 )
    {
        fprintf( stderr, "unable to bind: %s\n", strerror( errno ));
        return 1;
    }

    last = time(0);

    while ( 1 )
    {
        socklen_t  salen = sizeof(sa);
        int v;
        fd_set rfds;

        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);

        tv.tv_sec = 1;
        tv.tv_usec = 0;
        if (select(fd+1, &rfds, NULL, NULL, &tv) <= 0)
        {
            flush_files();
            continue;
        }

        len = recvfrom( fd, buf+40, sizeof(buf)-40, /*flags*/ 0, 
                        (struct sockaddr *)&sa, &salen );

        gettimeofday( &tv, NULL );
        localtime_r( &tv.tv_sec, &tm_time );
        v = strftime( timestr, 30, "'%Y/%m/%d %H:%M:%S ", &tm_time );
        v += sprintf(timestr+v, "%d.%d.%d.%d: ",
                srcaddr[0], srcaddr[1], srcaddr[2], srcaddr[3]);

        len = strip_chars(buf+40,len);

        memcpy(buf+40-v, timestr, v);

        process_line( buf+40-v, len+v, buf+40, len );
        now = time(0);
        if ( last != now )
        {
            flush_files();
            last = now;
        }
    }
}
