
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
#include <fcntl.h>

#include <iomanip>

#include "PageCache.h"
#include "PageIO.h"
#include "posix_fe.h"

PageIODirectoryTree :: PageIODirectoryTree(
    const std::string &_encryption_password,
    const std::string &_dirname, bool create)
    : PageIO(_encryption_password),
      ok(false), dirname(_dirname)
{
    // this will probably cause problems for someone.
    if (sizeof(off_t) != 8)
    {
        fprintf(stderr, "\n\n\nERROR : size of off_t is not 8! \n\n\n");
        exit(1);
    }

    pgsize = ciphering_enabled ? CIPHERED_PAGE_SIZE : PCP_PAGE_SIZE;

    options = O_RDWR | O_CREAT;
#ifdef O_LARGEFILE
    // required on cygwin?
    options |= O_LARGEFILE;
#endif

    if (dirname[0] != '/')
    {
        // relative to cwd
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) == NULL)
        {
            int e = errno;
            char * err = strerror(errno);
            fprintf(stderr, "getcwd failed: %d (%s)\n", e, err);
        }
        std::string newname = std::string(cwd) + "/" + dirname;
        dirname = newname;
    }

    if (create)
    {
        if (mkdir(dirname.c_str(), 0700) < 0)
        {
            int e = errno;
            char * err = strerror(e);
            fprintf(stderr, "PageIODirectoryTree: mkdir '%s': %d (%s)\n",
                    dirname.c_str(), e, err);
            return;
        }
        ok = true;
    }
    else
    {
        pxfe_readdir d;
        uint64_t pgfnum = 0;
        while (1)
        {
            pagefile * pf = new pagefile(pgfnum);
            std::string fname = dirname + "/" + pf->relpath;
            struct stat sb;
            if (::stat(fname.c_str(), &sb) < 0)
            {
                delete pf;
                break;
            }
            pagefile_list.add_tail(pf);
            pagefile_hash.add(pf);
            pgfnum ++;
        }

        ok = true;
    }
}

//virtual
PageIODirectoryTree :: ~PageIODirectoryTree( void )
{
    while (pagefile_list.get_cnt() > 0)
    {
        pagefile * f = pagefile_list.dequeue_head();
        pagefile_hash.remove(f);
        if (pagefile_lru.onthislist(f))
            pagefile_lru.remove(f);
        delete f;
    }
}

PageIODirectoryTree :: pagefile :: pagefile(uint64_t _pagenumber)
    : fd(-1), pagenumber(_pagenumber)
{
    relpath = format_relpath(pagenumber);
}

PageIODirectoryTree :: pagefile :: ~pagefile(void)
{
    close();
}

//static
std::string
PageIODirectoryTree :: pagefile :: format_relpath(uint64_t _pagenumber)
{
    uint64_t  x = _pagenumber;
    uint32_t level_1 = x & 0xfff;
    x >>= 12;
    uint32_t level_2 = x & 0xfff;
    x >>= 12;
    uint32_t level_3 = x; // whatever's left over

    std::ostringstream fname_str;
    fname_str << std::hex << std::setw(3) << std::setfill('0')
              << level_3;
    fname_str << "/"
              << std::hex << std::setw(3) << std::setfill('0')
              << level_2;
    fname_str << "/"
              << std::hex << std::setw(3) << std::setfill('0')
              << level_1;
    return fname_str.str();
}

static void
mkdir_minus_p(const std::string &path)
{
    size_t pos = 0;
    while (1)
    {
        pos = path.find_first_of('/', pos);
        if (pos == std::string::npos)
            break;
        const std::string &shortpath = path.substr(0,pos);
        if (shortpath != ".")
            mkdir(shortpath.c_str(), 0700);
        pos++;
    }
}

bool
PageIODirectoryTree :: pagefile :: open(const std::string &dirname, int options)
{
    if (fd > 0)
        return true;

    std::string fname = dirname + "/" + relpath;
    mkdir_minus_p(fname);
    int new_fd = ::open(fname.c_str(),options,0600);
    if (new_fd < 0)
    {
        int e = errno;
        char * err = strerror(e);
        fprintf(stderr, "PageIODirectoryTree :: get_pagefile: "
                "open '%s' failed: %d (%s)\n",
                fname.c_str(), e, err);
        return false;
    }
    fd = new_fd;
    return true;
}

void
PageIODirectoryTree :: pagefile :: close(void)
{
    if (fd > 0)
        ::close(fd);
    fd = -1;
}

uint64_t
PageIODirectoryTree :: pagefile_number(uint64_t page_number)
{
    return page_number / pgsize;
}

uint64_t
PageIODirectoryTree :: pagefile_page(uint64_t page_number)
{
    return page_number % pgsize;
}

PageIODirectoryTree::pagefile *
PageIODirectoryTree :: get_pagefile(const PageCachePage *pg, uint64_t &pgfpg)
{
    uint64_t page_number = pg->get_page_number();
    uint64_t pgfnum = pagefile_number(page_number);
    pgfpg = pagefile_page(page_number);
    pagefile * pf;

    while (pagefile_lru.get_cnt() >= max_file_pages)
    {
        pf = pagefile_lru.dequeue_tail();
        pf->close();
    }

    pf = pagefile_hash.find(pgfnum);
    if (pf == NULL)
    {
        pf = new pagefile(pgfnum);
        pagefile_list.add_tail(pf);
        pagefile_hash.add(pf);
    }
    else
    {
        // we're going to promote this pagefile on the LRU
        // if it's already on, so remove it from wherever it is.
        if (pagefile_lru.onthislist(pf))
            pagefile_lru.remove(pf);
    }

    if (pf->open(dirname,options) == false)
        // open has already printed an error.
        return NULL;

    pagefile_lru.add_head(pf);
    return pf;
}

//virtual
bool
PageIODirectoryTree :: get_page( PageCachePage * pg )
{
    uint64_t pgfpg;
    pagefile * pf = get_pagefile(pg, pgfpg);
    if (pf == NULL)
    {
        // fuxors
        fprintf(stderr, "PageIODirectoryTree :: get_page: shouldn't happen\n");
        return false;
    }

    uint8_t hold_buffer[CIPHERED_PAGE_SIZE];
    off_t offset = (off_t)pgfpg * pgsize;
    lseek(pf->fd, offset, SEEK_SET);
    uint8_t * bufptr = ciphering_enabled ? hold_buffer : pg->get_ptr();
    int cc = read(pf->fd, bufptr, pgsize);
    if (cc < 0)
    {
        fprintf(stderr,
                "PageIOFileDescriptor :: get_page: read -> %s\n",
                strerror(errno));
        return false;
    }
    if (cc != (int) pgsize)
    {
        // zero-fill the remainder of the page.
        memset(bufptr + cc, 0, pgsize - cc);
        if (ciphering_enabled)
            return true;
    }
    if (ciphering_enabled)
        decrypt_page(pgfpg, pg->get_ptr(), hold_buffer);
    return true;
}

//virtual
bool
PageIODirectoryTree :: put_page( PageCachePage * pg )
{
    uint64_t pgfpg;
    pagefile * pf = get_pagefile(pg, pgfpg);
    if (pf == NULL)
    {
        // fuxors
        fprintf(stderr, "PageIODirectoryTree :: get_page: shouldn't happen\n");
        return false;
    }

    uint8_t hold_buffer[CIPHERED_PAGE_SIZE];
    off_t offset = (off_t)pgfpg * pgsize;
    lseek(pf->fd, offset, SEEK_SET);
    uint8_t * bufptr = NULL;
    if (ciphering_enabled)
    {
        encrypt_page(pgfpg, hold_buffer, pg->get_ptr());
        bufptr = hold_buffer;
    }
    else
        bufptr = pg->get_ptr();
    if (write(pf->fd, bufptr, pgsize) != pgsize)
    {
        fprintf(stderr,
                "PageIOFileDescriptor :: put_page: write: %s\n",
                strerror(errno));
        return false;
    }
    return true;
}

//virtual
void
PageIODirectoryTree :: truncate_pages(uint64_t num_pages)
{
    uint64_t pgfnum = pagefile_number(num_pages);
    uint64_t pgfpg = pagefile_page(num_pages);
    off_t offset = (off_t)pgfpg * pgsize;
    std::string fname;

    // truncate the partial pagefile containing "num_pages".
    pagefile * pf = pagefile_hash.find(pgfnum);
    if (pf)
    {
        pf->close();
        if (pagefile_lru.onthislist(pf))
            pagefile_lru.remove(pf);
        fname = dirname + "/" + pf->relpath;
        if (truncate(fname.c_str(), offset) < 0)
        {
            int e = errno;
            fprintf(stderr, "truncate %s failed: %d (%s)\n",
                    fname.c_str(), e, strerror(e));
        }
    }

    // remove all pagefiles higher than the one containing "num_pages".
    while (1)
    {
        pgfnum++;
        pf = pagefile_hash.find(pgfnum);
        if (pf == NULL)
            break;
        pf->close();
        if (pagefile_lru.onthislist(pf))
            pagefile_lru.remove(pf);
        fname = dirname + "/" + pf->relpath;
        if (unlink(fname.c_str()) < 0)
        {
            int e = errno;
            fprintf(stderr, "unlink %s failed: %d (%s)\n",
                    fname.c_str(), e, strerror(e));
        }
        pagefile_list.remove(pf);
        pagefile_hash.remove(pf);
        delete pf;
    }
}
