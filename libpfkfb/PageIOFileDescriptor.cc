
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

#include "PageCache.h"
#include "PageIO.h"


PageIOFileDescriptor :: PageIOFileDescriptor( int _fd )
{
    // this will probably cause problems for someone.
    if (sizeof(off_t) != 8)
    {
        fprintf(stderr, "\n\n\nERROR : size of off_t is not 8! \n\n\n");
        exit(1);
    }
    fd = _fd;
}

//virtual
PageIOFileDescriptor :: ~PageIOFileDescriptor( void )
{
    close(fd);
}

//virtual
bool
PageIOFileDescriptor :: get_page( PageCachePage * pg )
{
    int page = pg->get_page_number();
    off_t offset = (off_t)page * (off_t)PageCache::PC_PAGE_SIZE;
    lseek(fd, offset, SEEK_SET);
    int cc = read(fd, pg->get_ptr(), PageCache::PC_PAGE_SIZE);
    if (cc < 0)
    {
        fprintf(stderr,
                "PageIOFileDescriptor :: get_page: read -> %s\n",
                strerror(errno));
        return false;
    }
    if (cc != PageCache::PC_PAGE_SIZE)
    {
        // zero-fill the remainder of the page.
        memset(pg->get_ptr() + cc, 0, PageCache::PC_PAGE_SIZE - cc);
    }
    return true;
}

//virtual
bool
PageIOFileDescriptor :: put_page( PageCachePage * pg )
{
    int page = pg->get_page_number();
    off_t offset = (off_t)page * (off_t)PageCache::PC_PAGE_SIZE;
    lseek(fd, offset, SEEK_SET);
    if (write(fd, pg->get_ptr(),
              PageCache::PC_PAGE_SIZE) != PageCache::PC_PAGE_SIZE)
    {
        fprintf(stderr,
                "PageIOFileDescriptor :: put_page: write: %s\n",
                strerror(errno));
        return false;
    }
    return true;
}

//virtual
int
PageIOFileDescriptor :: get_num_pages(bool * page_aligned)
{
    struct stat sb;
    if (fstat(fd, &sb) < 0)
        return -1;
    if ((sb.st_size & (PageCache::PC_PAGE_SIZE -1 )) != 0)
    {
        if (page_aligned)
            *page_aligned = false;
        return (sb.st_size / PageCache::PC_PAGE_SIZE) + 1;
    }
    // else
    if (page_aligned)
        *page_aligned = true;
    return (sb.st_size / PageCache::PC_PAGE_SIZE);
}

//virtual
off_t
PageIOFileDescriptor :: get_size(void)
{
    struct stat sb;
    if (fstat(fd, &sb) < 0)
        return -1;
    return sb.st_size;
}

//virtual
void
PageIOFileDescriptor :: truncate_pages(int num_pages)
{
    off_t size = (off_t)num_pages * PageCache::PC_PAGE_SIZE;
    ftruncate(fd, size);
}
