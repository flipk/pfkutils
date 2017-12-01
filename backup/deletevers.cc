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
#include <iostream>
using namespace std;

void
bakFile::deletevers(void)
{
    int versionindex;
    uint32_t version;

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

    if (dbi.dbinfo_version() != CURRENT_DBINFO_VERSION)
    {
        cerr << "NOTICE : dbinfo version mismatch "
             << dbi.dbinfo_version() << " != "
             << CURRENT_DBINFO_VERSION
             << " set OVERRIDE_VERSION=1 to force"
             << endl;
        if (getenv("OVERRIDE_VERSION") == NULL)
            return;
        cerr << " (OVERRIDE_VERSION found, continuing)" << endl;
    }

    bakDatum newdbinfo(bt);
    newdbinfo.key_dbinfo();
    newdbinfo.data.dbinfo.dbinfo_version() = CURRENT_DBINFO_VERSION;
    newdbinfo.data.dbinfo.sourcedir() = dbi.sourcedir();
    newdbinfo.data.dbinfo.nextver() = dbi.nextver();
    newdbinfo.data.dbinfo.versions.resize(dbi.versions.length());
    int newversionsindex = 0;

    // first ensure every version the user specified
    // is actually in the database.
    for (versionindex = 0; versionindex < (int) opts.versions.size(); versionindex++)
    {
        version = opts.versions[versionindex];
        bool found = false;
        for (int ind = 0; ind < dbi.versions.length(); ind++)
            if (dbi.versions[ind]() == version)
            {
                found = true;
                break;
            }
        if (!found)
        {
            cerr << "error: version " << version << " is not found\n";
            return;
        }
    }

    // next build a new dbinfo with the selected versions removed
    for (versionindex = 0; versionindex < dbi.versions.length(); versionindex++)
    {
        version = dbi.versions[versionindex]();
        bool found = false;
        for (uint32_t ind = 0; ind < opts.versions.size(); ind++)
            if ((uint32_t) opts.versions[ind] == version)
            {
                found = true;
                break;
            }
        if (!found)
        {
            newdbinfo.data.dbinfo.versions[newversionsindex++]() =
                version;
        }
    }

    // now actually delete the versions.
    for (versionindex = 0; versionindex < (int) opts.versions.size(); versionindex++)
    {
        version = opts.versions[versionindex];
        delete_version(version);
    }

    newdbinfo.data.dbinfo.versions.resize(newversionsindex);
    newdbinfo.mark_dirty();
}

void
bakFile::delete_version(int version)
{
    bakDatum versioninfo(bt);
    versioninfo.key_versioninfo( version );
    if (versioninfo.get() == false)
    {
        cerr << "cant fetch versioninfo\n";
        return;
    }
    versioninfo.del();

    bakDatum versionindex(bt);
    uint32_t group = 0;

    while (1)
    {
        versionindex.key_versionindex( version, group );
        if (versionindex.get() == false)
            break;

        versionindex.del();

        const bakData::versionindex_data &vind =
            versionindex.data.versionindex;

        for (int find = 0; find < vind.filenames.length(); find++)
        {
            const string &fname = vind.filenames[find]();

            bakDatum fileinfo(bt);
            fileinfo.key_fileinfo( version, fname );
            if (fileinfo.get() == false)
            {
                cerr << "unable to get fileinfo\n";
                continue;
            }
            fileinfo.del();

            const bakData::fileinfo_data &fid = fileinfo.data.fileinfo;

            if (fid.link_contents().size() > 0)
            {
                // symlink objects don't have a corresponding blob chain,
                // so we're done with this file.
                continue;
            }

            bakDatum blobhash(bt);
            blobhash.key_blobhash( fid.hash(), fid.filesize() );
            if (blobhash.get() == false)
            {
                cerr << "unable to get blobhash\n";
                continue;
            }
            bakData::blobhash_data &bhd = blobhash.data.blobhash;
            FB_AUID_T auid = bhd.first_auid();

            if (bhd.refcount() == 1)
            {
                blobhash.del();
            }
            else
            {
                bhd.refcount() --;
                blobhash.mark_dirty();
                // dont delete the fileContents if the blobhash
                // still exists with a lower refcount.
                auid = 0;
            }

            bakFileContents bfc;

            while (auid != 0)
            {
                FileBlock * fb = fbi_data->get(auid);
                if (!fb)
                    break;
                if (bfc.bst_decode(fb->get_ptr(), fb->get_size()) == false)
                    break;
                fbi_data->release(fb);
                fbi_data->free(auid);
                auid = bfc.next_auid();
                bfc.bst_free();
            }
        }

        group++;
    }
}
