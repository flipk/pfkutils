
/** \file PageIO.C
 * \brief Implements PageIOFileDescriptor.
 * \author Phillip F Knaack */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "PageCache.H"
#include "PageIO.H"

PageIOFileDescriptor :: PageIOFileDescriptor( int _fd )
{
    fd = _fd;
}

PageIOFileDescriptor :: ~PageIOFileDescriptor( void )
{
    // nothing, note fd is not closed!
}

bool
PageIOFileDescriptor :: get_page( PageCachePage * pg )
{
    off_t offset = (off_t)pg->get_page_number() * (off_t)PageCache::PAGE_SIZE;
    lseek(fd, offset, SEEK_SET);
    int cc = read(fd, pg->get_ptr(), PageCache::PAGE_SIZE);
    if (cc < 0)
        return false;
    if (cc != PageCache::PAGE_SIZE)
    {
        // zero-fill the remainder of the page.
        memset(pg->get_ptr() + cc, 0, PageCache::PAGE_SIZE - cc);
    }
    return true;
}

bool
PageIOFileDescriptor :: put_page( PageCachePage * pg )
{
    off_t offset = (off_t)pg->get_page_number() * (off_t)PageCache::PAGE_SIZE;
    lseek(fd, offset, SEEK_SET);
    if (write(fd, pg->get_ptr(),
              PageCache::PAGE_SIZE) != PageCache::PAGE_SIZE)
        return false;
    return true;
}

int
PageIOFileDescriptor :: get_num_pages(bool * page_aligned)
{
    struct stat sb;
    if (fstat(fd, &sb) < 0)
        return -1;
    if ((sb.st_size & (PageCache::PAGE_SIZE -1 )) != 0)
    {
        if (page_aligned)
            *page_aligned = false;
        return (sb.st_size / PageCache::PAGE_SIZE) + 1;
    }
    // else
    if (page_aligned)
        *page_aligned = true;
    return (sb.st_size / PageCache::PAGE_SIZE);
}

off_t
PageIOFileDescriptor :: get_size(void)
{
    struct stat sb;
    if (fstat(fd, &sb) < 0)
        return -1;
    return sb.st_size;
}
