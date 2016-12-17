/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

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

void
tarfile_emit_fileheader(int fd, const std::string &path, uint64_t filesize)
{
    char header[512];

    if (path.size() > 99)
    {
        // must emit ././@LongLink shit
        populate_header(header, "././@LongLink", path.size(), true);
        if ((::write(fd, header, 512) != 512) ||
            (::write(fd, path.c_str(), path.size()) != path.size()))
        {
            fprintf(stderr, "tar emit fileheader: write failed\n");
        }
        tarfile_emit_padding(fd, path.size());
    }
    populate_header(header, path, (uint32_t) filesize, false);
    if (::write(fd, header, 512) != 512)
        fprintf(stderr, "tar emit fileheader: write failed\n");
}

void
tarfile_emit_padding(int fd, uint64_t filesize)
{
    uint64_t lastpage = filesize % 512;
    if (lastpage > 0)
    {
        int padding = 512 - lastpage;
        char buf[512];
        memset(buf, 0, padding);
        if (::write(fd, buf, padding) != padding)
            fprintf(stderr, "tar emit padding: write failed\n");
    }
}

void
tarfile_emit_footer(int fd)
{
    char buf[512 * 2];
    memset(buf, 0, sizeof(buf));
    if (::write(fd, buf, sizeof(buf)) != sizeof(buf))
        fprintf(stderr, "tar emit footer: write failed\n");
}
