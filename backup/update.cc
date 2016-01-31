/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#include "bakfile.h"
#include "database_items.h"
#include "sha256.h"
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>

#include <iostream>
#include <list>

using namespace std;

void
bakFile::update(void)
{
    bt = Btree::openFile(opts.backupfile.c_str(), CACHE_SIZE);
    if (bt == NULL)
    {
        cerr << "unable to open database\n";
        return;
    }

    if (opts.verbose > 1)
        cout << "database opened\n";

    _update();
}

bool
bakFile::calc_file_hash(string &hash, const string &path)
{
    hash.resize(32);
    unsigned char * hash_buffer = (unsigned char *) hash.c_str();
    if (sha256_file(path.c_str(), hash_buffer, /*is224*/0) != 0)
    {
        cerr << "unable to sha256 file: " << path << endl;
        hash.resize(0);
        return false;
    }
    if (opts.verbose > 0)
        cout << "hash " << format_hash(hash) << " " << path << endl;
    return true;
}

bool
bakFile :: put_file(string &hash, const string &path, const uint64_t filesize)
{
    if (calc_file_hash(hash, path) == false)
        return false;

    bakDatum blobhash(bt);

    blobhash.key.which.v = bakKey::BLOBHASH;
    blobhash.key.blobhash.hash.string = hash;
    blobhash.key.blobhash.filesize.v = filesize;
    if (blobhash.get() == false)
    {
        int fd = ::open(path.c_str(), O_RDONLY);
        if (fd < 0)
        {
            cerr << "can't open file " << path
                 << " even though we just hashed it!\n";
            return false;
        }

#define MYBUFLEN 65528 // BST_STRING has a bug, can't do >= 65536

        struct filecont {
            bakFileContents bfc;
            FB_AUID_T auid;
            int read(int fd) {
                bfc.data.string.resize(MYBUFLEN);
                int cc = ::read(fd,
                                (char*) bfc.data.string.c_str(),
                                MYBUFLEN);
                if (cc >= 0 && cc != MYBUFLEN)
                    bfc.data.string.resize(cc);
                return cc;
            }
            void alloc(FileBlockInterface * fbi) {
                auid = fbi->alloc(bfc.bst_calc_size());
            }
            void set_next(filecont *oc) {
                if (oc == NULL)
                    bfc.next_auid.v = 0;
                else
                    bfc.next_auid.v = oc->auid;
            }
            void store(FileBlockInterface * fbi) {
                FileBlock * fb = fbi->get(auid,true);
                int len = fb->get_size();
                bfc.bst_encode(fb->get_ptr(), &len);
                fbi->release(fb,true);
            }
        };
        filecont conts[2];
        int ind = 0, cc;
        bool first_block = true;

        while (1) {
            filecont * c = &conts[ind];
            filecont * oc = &conts[ind^1];

            cc = c->read(fd);
            if (cc <= 0)
            {
                if (first_block)
                {
                    // zero length file!
                    c->alloc(fbi);
                    blobhash.data.blobhash.first_auid.v = c->auid;
                    c->set_next(NULL);
                    c->store(fbi);
                }
                else
                {
                    oc->set_next(NULL);
                    oc->store(fbi);
                }
                break;
            }
            c->alloc(fbi);
            if (first_block)
            {
                blobhash.data.blobhash.first_auid.v = c->auid;
                first_block = false;
            }
            else
            {
                oc->set_next(c);
                oc->store(fbi);
            }
            ind ^= 1;
        }

        close(fd);

        blobhash.data.which.v = bakData::BLOBHASH;
        blobhash.data.blobhash.refcount.v = 1;

        if (opts.verbose > 1)
            cout << "created new blob " << format_hash(hash) << endl;
    }
    else
    {
        blobhash.data.blobhash.refcount.v++;
        if (opts.verbose > 1)
            cout << "bumped refcount on blob " << format_hash(hash) << endl;
    }

    blobhash.mark_dirty();

    return true;
}

struct fileInfo {
    string name;
    struct stat sb;
    bool err;
    myTimeval mtime;
    fileInfo(const string &_name) : name(_name) {
        if (lstat(name.c_str(), &sb) < 0) {
            int e = errno;
            cout << "unable to stat, error " << e << ": "
                 << strerror(e) << endl;
            err = true;
        } else {
            if (S_ISREG(sb.st_mode) || S_ISDIR(sb.st_mode))
            {
                mtime.tv_sec = sb.st_mtim.tv_sec;
                mtime.tv_usec = sb.st_mtim.tv_nsec / 1000;
                err = false;
            }
            else
            {
                cerr << "unsupported: " << name << endl;
                // we only handle files, no special types.
                err = true;
            }
        }
    }
    const bool isdir(void) const { return S_ISDIR(sb.st_mode); }
};

void
bakFile::_update(void)
{
    uint32_t version, prev_version = 0, file_count = 0;
    uint64_t total_bytes = 0;
    bakDatum  dbinfo(bt);
    string sourcedir;
    myTimeval now;

    fbi = bt->get_fbi();

    now.getNow();

    dbinfo.key.which.v = bakKey::DBINFO;
    dbinfo.key.dbinfo.init();
    if (dbinfo.get() == false)
    {
        cerr << "bogus database: missing DBINFO\n";
        return;
    }

    version = dbinfo.data.dbinfo.nextver.v;
    int version_index = dbinfo.data.dbinfo.versions.num_items - 1;
    if (version_index >= 0)
        prev_version = dbinfo.data.dbinfo.versions.array[version_index]->v;
    sourcedir = dbinfo.data.dbinfo.sourcedir.string;

    if (chdir(sourcedir.c_str()) < 0)
    {
        cerr << "unable to chdir to " << sourcedir << endl;
        return;
    }

    if (opts.verbose > 1)
        cout << "creating new version " << version
             << " from prev version " << prev_version << endl;

    std::list<fileInfo> todo;
    todo.push_back(fileInfo("."));

    bakDatum versionindex(bt);
    versionindex.key.which.v = bakKey::VERSIONINDEX;
    versionindex.key.versionindex.version.v = version;
    versionindex.key.versionindex.group.v = 0;
    versionindex.data.which.v = bakData::VERSIONINDEX;
    versionindex.data.versionindex.filenames.alloc(
        bakData::versionindex_data::MAX_FILENAMES);
    int vind = 0;

    while (todo.size() > 0)
    {
        const fileInfo &item = todo.front();
        if (item.isdir())
        {
            DIR * d = opendir(item.name.c_str());
            if (d)
            {
                struct dirent * de;
                while ((de = readdir(d)) != NULL)
                {
                    string dename(de->d_name);
                    if (dename == "." || dename == "..")
                        continue;
                    todo.push_back(fileInfo(item.name + "/" + dename));
                    if (todo.back().err)
                        // take it off again
                        todo.pop_back();
                }
                closedir(d);
            }
        }
        else
        {
            string hash;
            file_count++;
            total_bytes += item.sb.st_size;

            bakDatum fileinfo(bt);
            fileinfo.key.which.v = bakKey::FILEINFO;
            fileinfo.key.fileinfo.version.v = prev_version;
            fileinfo.key.fileinfo.filename.string = item.name;
            if (fileinfo.get())
            {
                if (
              (fileinfo.data.fileinfo.time.btv_sec.v  != item.mtime.tv_sec ) ||
              (fileinfo.data.fileinfo.time.btv_usec.v != item.mtime.tv_usec) ||
              (fileinfo.data.fileinfo.filesize.v      != item.sb.st_size   )
                    )
                {
                    if (opts.verbose > 1)
                        cout << "file ts or hash changed: "
                             << item.name << endl;
                    put_file(hash, item.name, item.sb.st_size);
                }
                else
                {
                    hash = fileinfo.data.fileinfo.hash.string;
                    bakDatum blobhash(bt);
                    blobhash.key.which.v = bakKey::BLOBHASH;
                    blobhash.key.blobhash.hash.string = hash;
                    blobhash.key.blobhash.filesize.v = item.sb.st_size;
                    if (blobhash.get() == false)
                        cerr << "can't fetch blobhash\n";
                    else
                    {
                        if (opts.verbose > 1)
                            cout << "bumped refcount on blob "
                                 << format_hash(hash) << endl;
                        blobhash.data.blobhash.refcount.v++;
                        blobhash.mark_dirty();
                    }
                }  
            }
            else
            {
                put_file(hash, item.name, item.sb.st_size);
            }

            bakDatum newfinfo(bt);
            newfinfo.key.which.v = bakData::FILEINFO;
            newfinfo.key.fileinfo.version.v = version;
            newfinfo.key.fileinfo.filename.string = item.name;
            newfinfo.data.which.v = bakData::FILEINFO;
            newfinfo.data.fileinfo.hash.string     = hash;
            newfinfo.data.fileinfo.time.btv_sec.v  = item.mtime.tv_sec;
            newfinfo.data.fileinfo.time.btv_usec.v = item.mtime.tv_usec;
            newfinfo.data.fileinfo.filesize.v      = item.sb.st_size;
            newfinfo.mark_dirty();

            versionindex.data.versionindex.filenames.array[vind++]->string =
                item.name;
            versionindex.mark_dirty();

            if (vind == bakData::versionindex_data::MAX_FILENAMES)
            {
                if (opts.verbose > 1)
                    cout << "filled a versionindex\n";
                versionindex.put();
                versionindex.reinit();
                vind = 0;
                versionindex.key.versionindex.group.v++;
            }
        }
        todo.pop_front();
    }

    if (vind > 0 && vind < bakData::versionindex_data::MAX_FILENAMES)
    {
        if (opts.verbose > 1)
            cout << "resizing last versionindex to " << vind << endl;
        versionindex.data.versionindex.filenames.alloc(vind);
    }

    bakDatum versioninfo(bt);
    versioninfo.key.which.v = bakKey::VERSIONINFO;
    versioninfo.key.versioninfo.version.v = version;
    versioninfo.data.which.v = bakData::VERSIONINFO;
    versioninfo.data.versioninfo.time.btv_sec.v = now.tv_sec;
    versioninfo.data.versioninfo.time.btv_usec.v = now.tv_usec;
    versioninfo.data.versioninfo.filecount.v = file_count;
    versioninfo.data.versioninfo.total_bytes.v = total_bytes;
    versioninfo.mark_dirty();

    dbinfo.data.dbinfo.nextver.v++;
    version_index = dbinfo.data.dbinfo.versions.num_items;
    dbinfo.data.dbinfo.versions.alloc(version_index + 1);
    dbinfo.data.dbinfo.versions.array[version_index]->v = version;
    dbinfo.mark_dirty();
}
