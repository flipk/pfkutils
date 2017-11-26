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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <iostream>
#include <sstream>

using namespace std;

void
bakFile::listdb(void)
{
    if (openFiles() == false)
        return;

    bakDatum dbinfo(bt);
    dbinfo.key_dbinfo();
    if (dbinfo.get() == false)
    {
        cerr << "invalid database? cant fetch dbinfo\n";
        return;
    }

    cout << "dbinfo :\n"
         << "  sourcedir : " << dbinfo.data.dbinfo.sourcedir() << endl
         << "  nextver : " << dbinfo.data.dbinfo.nextver() << endl
         << "  versions :\n";
    for (int vind = 0; vind < dbinfo.data.dbinfo.versions.length(); vind++)
    {
        uint32_t version = dbinfo.data.dbinfo.versions[vind]();
        cout << "    ver: " << version << endl;
        bakDatum versioninfo(bt);
        versioninfo.key_versioninfo( version );
        if (versioninfo.get() == false)
            cerr << "can't fetch versioninfo\n";
        else
        {
            pxfe_timeval tv;
            versioninfo.data.versioninfo.time.get(tv);
            cout << "      time: " << format_time(tv) << endl;
            cout << "      filecount : "
                 << versioninfo.data.versioninfo.filecount() << endl;
            cout << "      total_bytes : "
                 << versioninfo.data.versioninfo.total_bytes() << endl;
            uint32_t vgroup = 0;
            if (opts.verbose > 0) while (1)
            {
                bakDatum versionindex(bt);
                versionindex.key_versionindex( version, vgroup );
                if (versionindex.get() == false)
                    break;
                const bakData::versionindex_data &vid =
                    versionindex.data.versionindex;
                cout << "        version group : " << vgroup << endl;
                for (int groupindex = 0;
                     groupindex < vid.filenames.length();
                     groupindex++)
                {
                    const string &fn =
                        vid.filenames[groupindex]();
                    cout << "          " << fn << endl;
                    if (opts.verbose > 1)
                    {
                        bakDatum fileinfo(bt);
                        fileinfo.key_fileinfo( version, fn );
                        if (fileinfo.get() == false)
                            cerr << "can't fetch fileinfo version "
                                 << version << " file "
                                 << fn << endl;
                        else
                        {
                            const bakData::fileinfo_data &fi =
                                fileinfo.data.fileinfo;
                            if (fi.link_contents().size() == 0)
                            {
                                cout << "            hash : "
                                     << format_hash(fi.hash()) << endl;
                                pxfe_timeval mtv;
                                fi.time.get(mtv);
                                cout << "            time : "
                                     << format_time(mtv) << endl;
                                cout << "            size : "
                                     << fi.filesize() << endl;

                                bakDatum blobhash(bt);
                                blobhash.key_blobhash( fi.hash(),
                                                       fi.filesize() );
                                if (blobhash.get() == false)
                                    cerr << "cant fetch blobhash\n";
                                else
                                {
                                    cout << "            blob ref : "
                                         << blobhash.data.blobhash.refcount()
                                         << endl
                                         << "            blob first auid : "
                                         << blobhash.data.blobhash.first_auid()
                                         << endl;
                                }
                            }
                            else
                            {
                                //symlink
                                cout << "            link target : "
                                     << fi.link_contents() << endl;
                            }
                        }
                    }
                }
                vgroup++;
            }
        }
    }
}
