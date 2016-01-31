/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#include "bakfile.h"
#include "database_items.h"
#include <iostream>
using namespace std;

void
bakFile::deletevers(void)
{
    int versionindex, version;

    bt = Btree::openFile(opts.backupfile.c_str(), CACHE_SIZE);
    if (bt == NULL)
    {
        cerr << "unable to open btree database\n";
        return;
    }
    fbi = bt->get_fbi();

    bakDatum dbinfo(bt);
    dbinfo.key.which.v = bakKey::DBINFO;
    dbinfo.key.dbinfo.init();
    if (dbinfo.get() == false)
    {
        cerr << "invalid database? cant fetch dbinfo\n";
        return;
    }
    const bakData::dbinfo_data &dbi = dbinfo.data.dbinfo;

    bakDatum newdbinfo(bt);
    newdbinfo.key.which.v = bakKey::DBINFO;
    newdbinfo.key.dbinfo.init();
    newdbinfo.data.which.v = bakData::DBINFO;
    newdbinfo.data.dbinfo.sourcedir.string = dbi.sourcedir.string;
    newdbinfo.data.dbinfo.nextver.v = dbi.nextver.v;
    newdbinfo.data.dbinfo.versions.alloc(dbi.versions.num_items);
    int newversionsindex = 0;

    // first ensure every version the user specified
    // is actually in the database.
    for (versionindex = 0; versionindex < opts.versions.size(); versionindex++)
    {
        version = opts.versions[versionindex];
        bool found = false;
        for (int ind = 0; ind < dbi.versions.num_items; ind++)
            if (dbi.versions.array[ind]->v == version)
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
    for (versionindex = 0; versionindex < dbi.versions.num_items; versionindex++)
    {
        version = dbi.versions.array[versionindex]->v;
        bool found = false;
        for (int ind = 0; ind < opts.versions.size(); ind++)
            if (opts.versions[ind] == version)
            {
                found = true;
                break;
            }
        if (!found)
        {
            newdbinfo.data.dbinfo.versions.array[newversionsindex++]->v =
                version;
        }
    }

    // now actually delete the versions.
    for (versionindex = 0; versionindex < opts.versions.size(); versionindex++)
    {
        version = opts.versions[versionindex];
        delete_version(version);
    }

    newdbinfo.data.dbinfo.versions.alloc(newversionsindex);
    newdbinfo.mark_dirty();
}

void
bakFile::delete_version(int version)
{
    bakDatum versioninfo(bt);
    versioninfo.key.which.v = bakKey::VERSIONINFO;
    versioninfo.key.versioninfo.version.v = version;
    if (versioninfo.get() == false)
    {
        cerr << "cant fetch versioninfo\n";
        return;
    }
    versioninfo.del();

    bakDatum versionindex(bt);
    versionindex.key.which.v = bakKey::VERSIONINDEX;
    versionindex.key.versionindex.version.v = version;
    versionindex.key.versionindex.group.v = 0;

    while (1)
    {
        if (versionindex.get() == false)
            break;

        versionindex.del();

        const bakData::versionindex_data &vind =
            versionindex.data.versionindex;

        for (int find = 0; find < vind.filenames.num_items; find++)
        {
            const string &fname = vind.filenames.array[find]->string;

            bakDatum fileinfo(bt);
            fileinfo.key.which.v = bakKey::FILEINFO;
            fileinfo.key.fileinfo.version.v = version;
            fileinfo.key.fileinfo.filename.string = fname;
            if (fileinfo.get() == false)
            {
                cerr << "unable to get fileinfo\n";
                continue;
            }
            fileinfo.del();

            const bakData::fileinfo_data &fid = fileinfo.data.fileinfo;

            bakDatum blobhash(bt);
            blobhash.key.which.v = bakKey::BLOBHASH;
            blobhash.key.blobhash.hash.string = fid.hash.string;
            blobhash.key.blobhash.filesize.v = fid.filesize.v;
            if (blobhash.get() == false)
            {
                cerr << "unable to get blobhash\n";
                continue;
            }
            bakData::blobhash_data &bhd = blobhash.data.blobhash;
            FB_AUID_T auid = bhd.first_auid.v;

            if (bhd.refcount.v == 1)
            {
                blobhash.del();
            }
            else
            {
                bhd.refcount.v --;
                blobhash.mark_dirty();
                // dont delete the fileContents if the blobhash
                // still exists with a lower refcount.
                auid = 0;
            }

            bakFileContents bfc;

            do {
                FileBlock * fb = fbi->get(auid);
                if (!fb)
                    break;
                if (bfc.bst_decode(fb->get_ptr(), fb->get_size()) == false)
                    break;
                fbi->release(fb);
                fbi->free(auid);
                auid = bfc.next_auid.v;
                bfc.bst_free();
            } while (auid != 0);
        }

        versionindex.key.versionindex.group.v++;
    }
}
