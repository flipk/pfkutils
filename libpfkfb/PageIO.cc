
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

/** \file PageIO.cc
 * \brief Implements PageIOFileDescriptor.
 * \author Phillip F Knaack */

/** \page PageIO PageIO object

The lowest layer is a derived object from PageIO.  This object knows
only how to read and write PageCachePage objects, whose body is of
size PageCache::PC_PAGE_SIZE.  An example implementation of PageIO is the
object PageIOFileDescriptor, which uses a file descriptor (presumably
an open file) to read and write offsets in the file.

If the user wishes some other storage mechanism (such as a file on a
remote server, accessed via RPC/TCP for example) then the user may
implement another PageIO backend which performs the necessary
interfacing.  This new PageIO object may be passed to the PageCache
constructor.

Next: \ref PageCache

*/

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>

#include "PageCache.h"
#include "PageIO.h"
#include "regex.h"

static const char * path_pattern = "\
^fbserver:([0-9]+.[0-9]+):([0-9]+.)$|\
^fbserver:([0-9]+.[0-9]+.[0-9]+.[0-9]+):([0-9]+.)$|\
^fbserver:(.+):([0-9]+.)$|\
^(.+)$";

enum {
    MATCH_ALL,
    MATCH_FBSRV_IP_2,
    MATCH_FBSRV_PORT_2,
    MATCH_FBSRV_IP_4,
    MATCH_FBSRV_PORT_4,
    MATCH_FBSRV_HOSTNAME,
    MATCH_FBSRV_PORT_HN,
    MATCH_FULLPATH,
    MAX_MATCHES
};

//static
PageIO *
PageIO :: open( const char * path, bool create, int mode )
{
    regex_t expr;
    regmatch_t matches[ MAX_MATCHES ];
    int regerr;
    char errbuf[80];

    regerr = regcomp( &expr, path_pattern, REG_EXTENDED );
    if (regerr != 0)
    {
        regerror( regerr, &expr, errbuf, sizeof(errbuf) );
        fprintf(stderr,"regcomp error: %s\n", errbuf);
        return NULL;
    }

    regerr = regexec( &expr, path, MAX_MATCHES, matches, 0 );
    if (regerr != 0)
    {
        regerror( regerr, &expr, errbuf, sizeof(errbuf) );
        fprintf(stderr,"regexec error: %s\n", errbuf);
        return NULL;
    }

    regfree( &expr );

    char string[512];
    struct in_addr ipaddr;
    struct hostent * he;
    int st, en, len, port;

    if (matches[MATCH_FBSRV_IP_2].rm_so != -1)
    {
        st = matches[MATCH_FBSRV_IP_2].rm_so;
        en = matches[MATCH_FBSRV_IP_2].rm_eo;
        len = en-st;
        if (len > 7)
        {
            printf("bogus ipaddr\n");
            return NULL;
        }
        memcpy(string, path + st, len);
        string[len] = 0;
        port = atoi(path + matches[MATCH_FBSRV_PORT_2].rm_so);
        inet_aton(string, &ipaddr);
        return new PageIONetworkTCPServer(&ipaddr, port);
    }
    else if (matches[MATCH_FBSRV_IP_4].rm_so != -1)
    {
        st = matches[MATCH_FBSRV_IP_4].rm_so;
        en = matches[MATCH_FBSRV_IP_4].rm_eo;
        len = en-st;
        if (len > 15)
        {
            printf("bogus ipaddr\n");
            return NULL;
        }
        memcpy(string, path + st, len);
        string[len] = 0;
        port = atoi(path + matches[MATCH_FBSRV_PORT_4].rm_so);
        inet_aton(string, &ipaddr);
        return new PageIONetworkTCPServer(&ipaddr, port);
    }
    else if (matches[MATCH_FBSRV_HOSTNAME].rm_so != -1)
    {
        st = matches[MATCH_FBSRV_HOSTNAME].rm_so;
        en = matches[MATCH_FBSRV_HOSTNAME].rm_eo;
        len = en-st;
        if (len > (int)sizeof(string))
        {
            fprintf(stderr, "error hostname too long?\n");
            return NULL;
        }
        memcpy(string, path + st, len);
        string[len] = 0;
        he = gethostbyname(string);
        if (he == NULL)
        {
            fprintf(stderr, "unknown host name '%s'\n", string);
            return NULL;
        }
        memcpy(&ipaddr.s_addr, he->h_addr, sizeof(ipaddr.s_addr));
        port = atoi(path + matches[MATCH_FBSRV_PORT_HN].rm_so);
        return new PageIONetworkTCPServer(&ipaddr, port);
    }
    else if (matches[MATCH_FULLPATH].rm_so != -1)
    {
        int options = O_RDWR;
        if (create)
            options |= O_CREAT | O_EXCL;
#ifdef O_LARGEFILE
        options |= O_LARGEFILE;
#endif
        int fd = ::open( path, options, mode );
        if (fd < 0)
            return NULL;
        return new PageIOFileDescriptor(fd);
    }
    else
    {
        fprintf(stderr, "PageIO::open: unknown PageIO method '%s'\n", path);
    }

    return NULL;
}
