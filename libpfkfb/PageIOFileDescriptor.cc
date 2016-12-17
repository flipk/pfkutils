
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


PageIOFileDescriptor :: PageIOFileDescriptor(
    const std::string &_encryption_password,
    int _fd )
    : PageIO(_encryption_password)
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
    uint8_t hold_buffer[CIPHERED_PAGE_SIZE];
    int page = pg->get_page_number();
    off_t pgsize = ciphering_enabled ? CIPHERED_PAGE_SIZE : PAGE_SIZE;
    off_t offset = (off_t)page * pgsize;
    lseek(fd, offset, SEEK_SET);
    uint8_t * bufptr = ciphering_enabled ? hold_buffer : pg->get_ptr();
    int cc = read(fd, bufptr, pgsize);
    if (cc < 0)
    {
        fprintf(stderr,
                "PageIOFileDescriptor :: get_page: read -> %s\n",
                strerror(errno));
        return false;
    }
    if (cc != pgsize)
    {
        // zero-fill the remainder of the page.
        memset(bufptr + cc, 0, pgsize - cc);
        if (ciphering_enabled)
            return true;
    }
    if (ciphering_enabled)
        decrypt_page(page, pg->get_ptr(), hold_buffer);
    return true;
}

//virtual
bool
PageIOFileDescriptor :: put_page( PageCachePage * pg )
{
    uint8_t hold_buffer[CIPHERED_PAGE_SIZE];
    int page = pg->get_page_number();
    off_t pgsize = ciphering_enabled ? CIPHERED_PAGE_SIZE : PAGE_SIZE;
    off_t offset = (off_t)page * pgsize;
    lseek(fd, offset, SEEK_SET);
    uint8_t * bufptr = NULL;
    if (ciphering_enabled)
    {
        encrypt_page(page, hold_buffer, pg->get_ptr());
        bufptr = hold_buffer;
    }
    else
        bufptr = pg->get_ptr();
    if (write(fd, bufptr, pgsize) != pgsize)
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
    off_t pgsize = ciphering_enabled ? CIPHERED_PAGE_SIZE : PAGE_SIZE;
    if (fstat(fd, &sb) < 0)
        return -1;
    if ((sb.st_size % pgsize) != 0)
    {
        if (page_aligned)
            *page_aligned = false;
        return (sb.st_size / pgsize) + 1;
    }
    // else
    if (page_aligned)
        *page_aligned = true;
    return (sb.st_size / pgsize);
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
    off_t pgsize = ciphering_enabled ? CIPHERED_PAGE_SIZE : PAGE_SIZE;
    off_t size = (off_t)num_pages * pgsize;
    if (ftruncate(fd, size) < 0)
        fprintf(stderr, "PageIOFileDescriptor :: truncate_pages: "
                "ftruncate failed\n");
}
