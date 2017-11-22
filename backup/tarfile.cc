/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

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

#include "tarfile.h"

#include <string.h>
#include <unistd.h>
#include <stdio.h>

using namespace std;

/*
   name         0               100             NUL-terminated if NUL fits
   mode         100             8
   uid          108             8
   gid          116             8
   size         124             12
   mtime        136             12
   chksum       148             8
   typeflag     156             1               see below
   linkname     157             100             NUL-terminated if NUL fits
   magic        257             6               must be TMAGIC (NUL term.)
   version      263             2               must be TVERSION
   uname        265             32              NUL-terminated
   gname        297             32              NUL-terminated
   devmajor     329             8
   devminor     337             8
   prefix       345             155             NUL-terminated if NUL fits
*/

// typeflag is '0' for regular files or 'L' for LongLink

static void
populate_header(char *header, const std::string &path,
                uint32_t size, bool longtype)
{
    memset(header, 0, 512);
    int pathlen = path.length();
    if (pathlen > 100)
        pathlen = 100;
    memcpy(header + 0, path.c_str(), pathlen);
    sprintf(header + 100, "0000644"); // mode
    sprintf(header + 108, "0000000"); // uid
    sprintf(header + 116, "0000000"); // gid
    sprintf(header + 124, "%011o", size);
    sprintf(header + 136, "00000000000");  // mtime
    memset(header + 148, ' ', 8);
    if (longtype)
        header[156] = 'L'; // typeflag -> LONGFILE
    else
        header[156] = '0'; // typeflag -> regular file
    sprintf(header + 257, "ustar  "); // magic
    sprintf(header + 265, "root" ); // uname
    sprintf(header + 297, "root" ); // gname

    uint32_t checksum = 0;
    unsigned char * buffer = (unsigned char *) header;
    for (int ind = 0; ind < 512; ind++)
        checksum += buffer[ind];
    sprintf(header + 148, "%06o", checksum);
}

static bool
do_write(int fd, const void *buf, size_t count)
{
    const char * ptr = (const char *) buf;
    while (count > 0)
    {
        errno = 0;
        int ret = ::write(fd, ptr, count);
        int e = errno;
        if (ret <= 0)
        {
            fprintf(stderr, "write returned %d: err %d: %s\n",
                    ret, e, strerror(e));
            return false;
        }
        ptr += ret;
        count -= ret;
    }
    return true;
}


bool
tarfile_emit_fileheader(int fd, const std::string &path, uint64_t filesize)
{
    char header[512];

    if (path.size() > 99)
    {
        // must emit ././@LongLink shit
        populate_header(header, "././@LongLink", path.size(), true);
        if ((do_write(fd, header, 512) == false) ||
            (do_write(fd, path.c_str(), path.size()) == false))
        {
            fprintf(stderr, "tar emit fileheader: write failed\n");
            return false;
        }
        if (tarfile_emit_padding(fd, path.size()) == false)
            return false;
    }
    populate_header(header, path, (uint32_t) filesize, false);
    if (do_write(fd, header, 512) == false)
    {
        fprintf(stderr, "tar emit fileheader: write failed\n");
        return false;
    }
    return true;
}

bool
tarfile_emit_padding(int fd, uint64_t filesize)
{
    uint64_t lastpage = filesize % 512;
    if (lastpage > 0)
    {
        int padding = 512 - lastpage;
        char buf[512];
        memset(buf, 0, padding);
        if (do_write(fd, buf, padding) == false)
        {
            fprintf(stderr, "tar emit padding: write failed\n");
            return false;
        }
    }
    return true;
}

bool
tarfile_emit_footer(int fd)
{
    char buf[512 * 2];
    memset(buf, 0, sizeof(buf));
    if (do_write(fd, buf, sizeof(buf)) == false)
    {
        fprintf(stderr, "tar emit footer: write failed\n");
        return false;
    }
    return true;
}
