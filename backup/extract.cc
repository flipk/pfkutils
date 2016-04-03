/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

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
        if (dbi.versions[cnt].v == version)
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
                const string &path = fns[ind].string;
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
    const string &hash = fileinfo.data.fileinfo.hash.string;
    uint64_t filesize = fileinfo.data.fileinfo.filesize.v;

    bakDatum blobhash(bt);
    blobhash.key_blobhash( hash, filesize );
    if (blobhash.get() == false)
    {
        cerr << "blobhash not found for " << path << endl;
        return;
    }

    FB_AUID_T auid = blobhash.data.blobhash.first_auid.v;

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
        if (bfc.data.string.length() > 0)
        {
            int cc = ::write(fd,
                             bfc.data.string.c_str(),
                             bfc.data.string.length());
            if (cc != bfc.data.string.length())
            {
                cerr << "unable to write to " << path << endl;
            }
        }
        auid = bfc.next_auid.v;
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
