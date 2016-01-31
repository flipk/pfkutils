/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#include "bakfile.h"
#include "database_items.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <iostream>
#include <sstream>

using namespace std;

void
bakFile::listdb(void)
{
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

    cout << "dbinfo :\n"
         << "  sourcedir : " << dbinfo.data.dbinfo.sourcedir.string << endl
         << "  nextver : " << dbinfo.data.dbinfo.nextver.v << endl
         << "  versions :\n";
    for (int vind = 0; vind < dbinfo.data.dbinfo.versions.num_items; vind++)
    {
        uint32_t version = dbinfo.data.dbinfo.versions.array[vind]->v;
        cout << "    ver: " << version << endl;
        bakDatum versioninfo(bt);
        versioninfo.key.which.v = bakKey::VERSIONINFO;
        versioninfo.key.versioninfo.version.v = version;
        if (versioninfo.get() == false)
            cerr << "can't fetch versioninfo\n";
        else
        {
            myTimeval tv;
            versioninfo.data.versioninfo.time.get(tv);
            cout << "      time: " << format_time(tv) << endl;
            cout << "      filecount : "
                 << versioninfo.data.versioninfo.filecount.v << endl;
            cout << "      total_bytes : "
                 << versioninfo.data.versioninfo.total_bytes.v << endl;
            int vgroup = 0;
            if (opts.verbose > 0) while (1)
            {
                bakDatum versionindex(bt);
                versionindex.key.which.v = bakKey::VERSIONINDEX;
                versionindex.key.versionindex.version.v = version;
                versionindex.key.versionindex.group.v = vgroup;
                if (versionindex.get() == false)
                    break;
                const bakData::versionindex_data &vid =
                    versionindex.data.versionindex;
                cout << "        version group : " << vgroup << endl;
                for (int groupindex = 0;
                     groupindex < vid.filenames.num_items;
                     groupindex++)
                {
                    const string &fn =
                        vid.filenames.array[groupindex]->string;
                    cout << "          " << fn << endl;
                    if (opts.verbose > 1)
                    {
                        bakDatum fileinfo(bt);
                        fileinfo.key.which.v = bakKey::FILEINFO;
                        fileinfo.key.fileinfo.version.v = version;
                        fileinfo.key.fileinfo.filename.string = fn;
                        if (fileinfo.get() == false)
                            cerr << "can't fetch fileinfo version "
                                 << fileinfo.key.fileinfo.version.v << " file "
                                 << fn << endl;
                        else
                        {
                            const bakData::fileinfo_data &fi =
                                fileinfo.data.fileinfo;
                            cout << "            hash : "
                                 << format_hash(fi.hash.string) << endl;
                            myTimeval mtv;
                            fi.time.get(mtv);
                            cout << "            time : "
                                 << format_time(mtv) << endl;
                            cout << "            size : "
                                 << fi.filesize.v << endl;

                            bakDatum blobhash(bt);
                            blobhash.key.which.v = bakKey::BLOBHASH;
                            blobhash.key.blobhash.hash.string = fi.hash.string;
                            blobhash.key.blobhash.filesize.v = fi.filesize.v;
                            if (blobhash.get() == false)
                                cerr << "cant fetch blobhash\n";
                            else
                            {
                                cout << "            blob ref : "
                                     << blobhash.data.blobhash.refcount.v
                                     << endl
                                     << "            blob first auid : "
                                     << blobhash.data.blobhash.first_auid.v
                                     << endl;
                            }
                        }
                    }
                }
                vgroup++;
            }
        }
    }
}
