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

bool
bakFile::match_file_name(const std::string &path)
{
    if (opts.paths.size() == 0)
        // if nothing was specified, all paths match
        return true;

    for (int ind = 0; ind < opts.paths.size(); ind++)
    {
        const string &pattern = opts.paths[ind];
        size_t len = pattern.length();
        if (path.compare(0,len,pattern) == 0)
        {
            // match!
            return true;
        }
    }

    // matched none of the patterns
    return false;
}

// if tarfd is -1 that means extract real files,
// otherwise write to a tar file.
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

    bakDatum versionindex(bt);
    uint32_t group = 0;
    bool fail = false;
    while (!fail)
    {
        versionindex.key_versionindex( version, group );
        if (versionindex.get() == false)
            break;
        const BST_ARRAY<BST_STRING> &fns =
            versionindex.data.versionindex.filenames;
        for (int ind = 0; ind < fns.length(); ind++)
        {
            const string &path = fns[ind]();
            if (match_file_name(path))
            {
                if (opts.verbose)
                {
                    cout << "x " << path;
                    cout.flush();
                }
                if (extract_file(version, path, tarfd) == false)
                {
                    fail = true;
                    break;
                }
                if (opts.verbose)
                    cout << endl;
            }
        }
        group ++;
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
bakFile :: extract_file(uint32_t version, const std::string &path, int tarfd)
{
    bakDatum fileinfo(bt);
    fileinfo.key_fileinfo( version, path );
    if (fileinfo.get() == false)
    {
        cerr << "version " << version << " file " << path << " not found\n";
        return false;
    }

    const string &link_cont = fileinfo.data.fileinfo.link_contents();
    if (link_cont.size() > 0)
    {
        if (opts.verbose)
        {
            cout << " --> " << link_cont;
            cout.flush();
        }
        mkdir_minus_p(path);
        if (symlink(link_cont.c_str(), path.c_str()) < 0)
        {
            int e = errno;
            char * err = strerror(e);
            if (opts.verbose == 0)
                cout << "link " << path << " --> "
                     << link_cont;
            cout << " : " << e << " (" << err << ")" << endl;
            return false;
        }
        return true;
    }

    const string &hash = fileinfo.data.fileinfo.hash();
    uint64_t filesize = fileinfo.data.fileinfo.filesize();

    bakDatum blobhash(bt);
    blobhash.key_blobhash( hash, filesize );
    if (blobhash.get() == false)
    {
        cerr << "blobhash not found for " << path << endl;
        return false;
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
            return false;
        }
    }
    else
    {
        if (tarfile_emit_fileheader(tarfd, path, filesize) == false)
        {
            return false;
        }
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
            if (do_write(fd,
                         bfc.data().c_str(),
                         bfc.data().length()) == false)
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
        if (tarfile_emit_padding(tarfd, filesize) == false)
            return false;
    }
    else
    {
        close(fd);
    }

    return true;
}
