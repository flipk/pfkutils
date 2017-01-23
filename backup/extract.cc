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

#include "bakfile.h"
#include "database_items.h"
#include "tarfile.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include <iostream>
using namespace std;

void
bakFile::extract(void)
{
    _extract(-1);
}

void
bakFile::_extract(int tarfd)
{
    uint32_t version = opts.versions[0];

    if (openFiles() == false)
        return;

    bakDatum dbinfo(bt);
    dbinfo.key_dbinfo();
    if (dbinfo.get() == false)
    {
        cerr << "invalid database? cant fetch dbinfo\n";
        return;
    }
    const bakData::dbinfo_data &dbi = dbinfo.data.dbinfo;

    bool found = false;
    for (int cnt = 0; cnt < dbi.versions.length(); cnt++)
        if (dbi.versions[cnt]() == version)
        {
            found = true;
            break;
        }

    if (!found)
    {
        cerr << "version " << version << " is not in this db\n";
        return;
    }

    if (opts.paths.size() == 0)
    {
        bakDatum versionindex(bt);
        uint32_t group = 0;
        while (1)
        {
            versionindex.key_versionindex( version, group );
            if (versionindex.get() == false)
                break;
            const BST_ARRAY<BST_STRING> &fns =
                versionindex.data.versionindex.filenames;
            for (int ind = 0; ind < fns.length(); ind++)
            {
                const string &path = fns[ind]();
                extract_file(version, path, tarfd);
            }
            group ++;
        }
    }
    else
    {
        for (int ind = 0; ind < opts.paths.size(); ind++)
        {
            const string &path = opts.paths[ind];
            extract_file(version, path, tarfd);
        }
    }

    if (tarfd > 0)
    {
        tarfile_emit_footer(tarfd);
        close(tarfd);
    }
}

static void
mkdir_minus_p(const string &path)
{
    size_t pos = 0;
    while (1)
    {
        pos = path.find_first_of('/', pos);
        if (pos == string::npos)
            break;
        const string &shortpath = path.substr(0,pos);
        if (shortpath != ".")
            mkdir(shortpath.c_str(), 0700);
        pos++;
    }
}

void
bakFile :: extract_file(uint32_t version, const std::string &path, int tarfd)
{
    bakDatum fileinfo(bt);
    fileinfo.key_fileinfo( version, path );
    if (fileinfo.get() == false)
    {
        cerr << "version " << version << " file " << path << " not found\n";
        return;
    }
    const string &hash = fileinfo.data.fileinfo.hash();
    uint64_t filesize = fileinfo.data.fileinfo.filesize();

    bakDatum blobhash(bt);
    blobhash.key_blobhash( hash, filesize );
    if (blobhash.get() == false)
    {
        cerr << "blobhash not found for " << path << endl;
        return;
    }

    FB_AUID_T auid = blobhash.data.blobhash.first_auid();

    if (tarfd < 0)
        mkdir_minus_p(path);

    bakFileContents bfc;
    int fd = -1;

    if (tarfd < 0)
    {
        fd = ::open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
        if (fd < 0)
        {
            int e = errno;
            cerr << "unable to open file " << path << ": "
                 << strerror(e) << endl;
            return;
        }
    }
    else
    {
        tarfile_emit_fileheader(tarfd, path, filesize);
        fd = tarfd;
    }

    while (auid != 0)
    {
        FileBlock * fb = fbi_data->get(auid);
        if (!fb)
            break;
        if (bfc.bst_decode(fb->get_ptr(), fb->get_size()) == false)
            break;
        if (bfc.data().length() > 0)
        {
            int cc = ::write(fd,
                             bfc.data().c_str(),
                             bfc.data().length());
            if (cc != bfc.data().length())
            {
                cerr << "unable to write to " << path << endl;
            }
        }
        auid = bfc.next_auid();
        bfc.bst_free();
        fbi_data->release(fb);
    }

    if (tarfd > 0)
    {
        tarfile_emit_padding(tarfd, filesize);
    }
    else
    {
        close(fd);
    }
}
