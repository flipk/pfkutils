
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
      dirname(_dirname), ok(false)
{
    // this will probably cause problems for someone.
    if (sizeof(off_t) != 8)
    {
        fprintf(stderr, "\n\n\nERROR : size of off_t is not 8! \n\n\n");
        exit(1);
    }

    pgsize = ciphering_enabled ? CIPHERED_PAGE_SIZE : PCP_PAGE_SIZE;
    num_pages = 0;
    options = O_RDWR | O_CREAT;
#ifdef O_LARGEFILE
    // required on cygwin?
    options |= O_LARGEFILE;
#endif

    if (dirname[0] != '/')
    {
        // relative to cwd
        char cwd[PATH_MAX];
        getcwd(cwd, sizeof(cwd));
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
        struct fentry;
        typedef DLL3::List<fentry,1,false,false> fentry_list_t;
        struct fentry : public fentry_list_t::Links {
            const std::string name;
            struct stat sb;
            bool statdone;
            fentry(const std::string &_name)
                : name(_name), statdone(false) { }
            fentry(const fentry &other)
                : name(other.name), statdone(other.statdone) {
                if (statdone)
                    memcpy(&sb, &other.sb, sizeof(sb));
            }
            bool dostat(void) {
                if (!statdone  &&  lstat(name.c_str(), &sb) == 0)
                    statdone = true;
                return statdone;
            }
        };
        fentry_list_t  fentries, dentries;

        dentries.add_tail(new fentry(dirname));

        while (dentries.get_cnt() > 0)
        {
            fentry *f = dentries.dequeue_head();
            if (d.open(f->name) == false)
            {
                int e = errno;
                char * err = strerror(e);
                fprintf(stderr, "PageIODirectoryTree: opendir '%s': %d (%s)\n",
                        dirname.c_str(), e, err);
            }
            else
            {
                dirent de;
                while (d.read(de))
                {
                    std::string dname = de.d_name;
                    if (dname == "." || dname == "..")
                        continue;
                    std::string fn = f->name + "/" + dname;
                    fentry * fe = new fentry(fn);
                    if (de.d_type == DT_UNKNOWN)
                    {
                        // unsupported by this fs, we'll have to
                        // stat to determine type.
                        if (fe->dostat() == true)
                        {
                            if (S_ISREG(fe->sb.st_mode))
                                de.d_type = DT_REG;
                            else if (S_ISDIR(fe->sb.st_mode))
                                de.d_type = DT_DIR;
                        }
                        else
                        {
                            int e = errno;
                            char * err = strerror(e);
                            fprintf(stderr, "PageIODirectoryTree: unable to "
                                    "stat %s: %d (%s)\n", fn.c_str(), e, err);
                        }
                    }
                    switch (de.d_type)
                    {
                    case DT_REG:
                        fe->dostat();
                        fentries.add_tail(fe);
                        break;
                    case DT_DIR:
                        dentries.add_tail(fe);
                        break;
                    default:
                        delete fe;
                        fprintf(stderr, "PageIODirectoryTree: ignoring "
                                "%s: unsupported DT_ type %d\n",
                                fn.c_str(), de.d_type);
                        /*ignore all other types*/;
                    }
                }
                d.close();
            }
            delete f;
        }

        while (fentries.get_cnt() > 0)
        {
            fentry *f = fentries.dequeue_head();
            num_pages += f->sb.st_size / pgsize;
            delete f;
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
        delete f;
    }
}

PageIODirectoryTree :: pagefile :: ~pagefile(void)
{
    if (fd > 0)
        close(fd);
}

uint64_t
PageIODirectoryTree :: pagefile_number(const PageCachePage *pg)
{
    return pg->get_page_number() / pgsize;
}

uint64_t
PageIODirectoryTree :: pagefile_page(const PageCachePage *pg)
{
    return pg->get_page_number() % pgsize;
}

PageIODirectoryTree::pagefile *
PageIODirectoryTree :: get_pagefile(const PageCachePage *pg, uint64_t &pgfpg)
{
    uint64_t pgfnum = pagefile_number(pg);
    pgfpg = pagefile_page(pg);
    pagefile * pf = pagefile_hash.find(pgfnum);
    if (pf == NULL)
    {
        while (pagefile_list.get_cnt() > max_file_pages)
        {
            pf = pagefile_list.dequeue_head();
            pagefile_hash.remove(pf);
            delete pf;
        }

        std::ostringstream fname_str;
        fname_str << dirname << "/";

        uint64_t  x = pgfnum;
        uint32_t level_1 = x & 0xfff;
        x >>= 12;
        uint32_t level_2 = x & 0xfff;
        x >>= 12;
        uint32_t level_3 = x; // whatever's left over

        fname_str << std::hex << std::setw(3) << std::setfill('0')
                  << level_3;
        mkdir(fname_str.str().c_str(),0700);
        fname_str << "/"
                  << std::hex << std::setw(3) << std::setfill('0')
                  << level_2;
        mkdir(fname_str.str().c_str(),0700);
        fname_str << "/"
                  << std::hex << std::setw(3) << std::setfill('0')
                  << level_1;

        int new_fd = ::open(fname_str.str().c_str(),options,0600);
        if (new_fd < 0)
        {
            int e = errno;
            char * err = strerror(e);
            fprintf(stderr, "PageIODirectoryTree :: get_pagefile: "
                    "open '%s' failed: %d (%s)\n",
                    fname_str.str().c_str(), e, err);
            return NULL;
        }
        pf = new pagefile(new_fd, pgfnum);
        pagefile_list.add_tail(pf);
        pagefile_hash.add(pf);
    }
    else
    {
        //promote this pagefile on the LRU
        pagefile_list.remove(pf);
        pagefile_list.add_tail(pf);
    }
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
    if (cc != pgsize)
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
uint64_t
PageIODirectoryTree :: get_num_pages(bool * page_aligned)
{
    // xxx this needs to be implemented
    return 9;
}

//virtual
off_t
PageIODirectoryTree :: get_size(void)
{
    // xxx this needs to be implemented
    return 9;
}

//virtual
void
PageIODirectoryTree :: truncate_pages(uint64_t num_pages)
{
    // xxx this needs to be implemented
}
