
/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
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
    uint64_t page = pg->get_page_number();
    off_t pgsize = ciphering_enabled ? CIPHERED_PAGE_SIZE : PCP_PAGE_SIZE;
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
    uint64_t page = pg->get_page_number();
    off_t pgsize = ciphering_enabled ? CIPHERED_PAGE_SIZE : PCP_PAGE_SIZE;
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
uint64_t
PageIOFileDescriptor :: get_num_pages(bool * page_aligned)
{
    struct stat sb;
    off_t pgsize = ciphering_enabled ? CIPHERED_PAGE_SIZE : PCP_PAGE_SIZE;
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
PageIOFileDescriptor :: truncate_pages(uint64_t num_pages)
{
    off_t pgsize = ciphering_enabled ? CIPHERED_PAGE_SIZE : PCP_PAGE_SIZE;
    off_t size = (off_t)num_pages * pgsize;
    if (ftruncate(fd, size) < 0)
        fprintf(stderr, "PageIOFileDescriptor :: truncate_pages: "
                "ftruncate failed\n");
}
