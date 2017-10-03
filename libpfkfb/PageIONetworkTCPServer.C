
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

/** \todo doxygen this file. */


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

#include "PageCache.H"
#include "PageIO.H"

/** \todo: implement these functions. */


PageIONetworkTCPServer :: PageIONetworkTCPServer(
    void *addr, // actually, struct in_addr *
    int port)
{
}

PageIONetworkTCPServer :: ~PageIONetworkTCPServer(void)
{
}

//virtual
bool
PageIONetworkTCPServer :: get_page( PageCachePage * pg )
{
    return false;
}

//virtual
bool
PageIONetworkTCPServer :: put_page( PageCachePage * pg )
{
    return false;
}

//virtual
int
PageIONetworkTCPServer :: get_num_pages(bool * page_aligned)
{
    return 0;
}

//virtual
off_t
PageIONetworkTCPServer :: get_size(void)
{
    return 0;
}

//virtual
void
PageIONetworkTCPServer :: truncate_pages(int num_pages)
{
}
