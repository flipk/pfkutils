/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#include "bakfile.h"
#include "database_items.h"
#include <iostream>
using namespace std;

void
bakFile::deletevers(void)
{
    int versionindex, version;

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

    bakDatum newdbinfo(bt);
    newdbinfo.key_dbinfo();
    newdbinfo.data.dbinfo.sourcedir() = dbi.sourcedir();
    newdbinfo.data.dbinfo.nextver() = dbi.nextver();
    newdbinfo.data.dbinfo.versions.resize(dbi.versions.length());
    int newversionsindex = 0;

    // first ensure every version the user specified
    // is actually in the database.
    for (versionindex = 0; versionindex < opts.versions.size(); versionindex++)
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
        for (int ind = 0; ind < opts.versions.size(); ind++)
            if (opts.versions[ind] == version)
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
    for (versionindex = 0; versionindex < opts.versions.size(); versionindex++)
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
