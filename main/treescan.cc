#if 0
set -e -x
objdir="obj.Linux-4.2.7-300.fc23.x86_64.blade"
incs="-I../libpfkfb -I../libpfkutil"
libs="../$objdir/libpfkfb.a ../$objdir/libpfkutil.a ../$objdir/libpfkdll2.a"
g++ -O0 -g3 $incs treescan.cc -Dtreescan_main=main $libs -o treescan
exit 0
    ;
#endif
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

#include "pfkutils_config.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>

#include <iostream>
#include <string>
#include <list>
#include <map>

#include "Btree.h"
#include "posix_fe.h"
#include "mbedtls/sha1.h"

#define TSDB_FILENAME ".tsdb"

using namespace std;

typedef std::list<std::string> ignoreList;

class treescan_tree {
private:
    struct tskey : public BST_UNION {
        enum { DBINFO, FNAME_TO_INFO, MAX };
        tskey(void)
            : BST_UNION(NULL,MAX), dbinfo(this), fname_to_info(this) { }
        BST_STRING  dbinfo; // DBINFO
        struct bst_fname_key : public BST {
            bst_fname_key(BST *parent)
                : BST(parent), fname(this) { }
            BST_STRING  fname;
        } fname_to_info; // FNAME_TO_INFO
    };
    struct tsdata : public BST_UNION {
        enum { DBINFO, FNAME_TO_INFO, MAX };
        tsdata(void)
            : BST_UNION(NULL,MAX), dbinfo(this), fname_to_info(this) { }
        struct bst_dbinfo : public BST {
            bst_dbinfo(BST * parent)
                : BST(parent), last_scan(this) {}
            BST_TIMEVAL last_scan;
            // more?
        } dbinfo; // DBINFO
        struct bst_fname_info : public BST {
            bst_fname_info(BST * parent)
                : BST(parent),
                  lastScanned(this), mtime(this), sha1hash(this) { }
            BST_TIMEVAL lastScanned;
            BST_TIMEVAL mtime;
            BST_STRING  sha1hash;
        } fname_to_info; // FNAME_TO_INFO
    };
    class tsdatum {
        Btree * bt;
        FB_AUID_T data_id;
        bool dirty;
    public:
        tsdatum(Btree * _bt) { data_id = 0; bt = _bt; dirty = false; }
        ~tsdatum(void) { if (dirty) put(); }
        // if you modify the data item, you should call this.
        void mark_dirty(void) { dirty = true; }
        // fill out the key item before calling this; if it
        // returns true, the data item is populated.
        bool get(void) {
            if (bt->get(&key,&data_id) == false)
                return false;
            return get_data(data_id);
        }
        bool get_data(FB_AUID_T id) {
            FileBlock * datafb = bt->get_fbi()->get(id,false);
            if (datafb == NULL)
            {
                cout << "bt->get_fbi()->get(data_id) failed!\n";
                return false;
            }
            bool ret = true;
            if (data.bst_decode(datafb->get_ptr(),
                                datafb->get_size()) == false)
            {
                cout << "data.bst_decode failed!\n";
                ret = false;
            }
            else
            {
                if (data.which() != key.which())
                {
                    cout << "data.which() != key.which()!\n";
                    ret = false;
                }
            }
            bt->get_fbi()->release(datafb,false);
            dirty = false;
            return ret;
        }
        // fill out key and data items before calling this.
        void put(void) {
            FileBlockInterface * fbi = bt->get_fbi();
            int datalen = data.bst_calc_size();
            if (data_id != 0)
                fbi->realloc(data_id, datalen);
            else
                data_id = fbi->alloc(datalen);
            FileBlock * fb = fbi->get(data_id,true);
            if (fb)
            {
                int len = fb->get_size();
                data.bst_encode(fb->get_ptr(),&len);
                fbi->release(fb,true);
            }
            dirty = false;
            bool replaced = false;
            FB_AUID_T old_data_id = 0;
            bt->put(&key, data_id, /*replace*/true,
                    &replaced, &old_data_id);
            if (replaced &&
                old_data_id != 0 &&
                old_data_id != data_id)
            {
                fbi->free(old_data_id);
            }
        }
        // fill out key before calling this.
        void del(void) {
            FB_AUID_T old_data_id = 0;
            if (bt->del(&key, &old_data_id) == false) {
                cout << "error deleting? what now?\n";
            } else {
                bt->get_fbi()->free(old_data_id);
            }
            dirty = false;
            data_id = 0;
        }
        tskey key;
        tsdata data;
    };
#define SHA1HashSize 20 // no define in mbedtls!
    struct Sha1Hash {
        static const int size = SHA1HashSize;
        unsigned char hash[size];
        inline bool operator<(const Sha1Hash &other) const {
            return memcmp(hash, other.hash, size) < 0;
        }
        std::string Format(void) {
            char str[size*2+1];
            for (int ind = 0; ind < size; ind++)
                sprintf(str + ind*2, "%02x", hash[ind]);
            return std::string(str);
        }
    };
    struct fileInfo {
        fileInfo(const string &_name) : name(_name) {
            if (lstat(name.c_str(), &sb) < 0) {
                int e = errno;
                cout << "unable to stat, error " << e << ": "
                     << strerror(e) << endl;
                err = true;
            } else {
                mtime.tv_sec = sb.st_mtim.tv_sec;
                mtime.tv_usec = sb.st_mtim.tv_nsec / 1000;
                err = false;
                //printf("mode of %s is %x\n", name.c_str(), sb.st_mode);
            }
        }
        const bool isdir(void) const { return S_ISDIR(sb.st_mode); }
        const bool islnk(void) const { return S_ISLNK(sb.st_mode); }
        string name;
        struct stat sb;
        bool err;
        pxfe_timeval mtime;
    };
    pxfe_timeval lastScan;
    const ignoreList &ignore_list;
    bool ignore_file(const string &fname) const {
        size_t slashpos = fname.find_last_of('/');
        if (slashpos == string::npos)
            slashpos = 0;
        else
            slashpos++;

// for (auto it : ignore_list) if we had good compiler on all archs
        for (ignoreList::const_iterator it = ignore_list.begin();
             it != ignore_list.end();
             it++)
        {
            if (fname.compare(slashpos, string::npos, *it) == 0)
                return true;
        }
        return false;
    }
public:
    treescan_tree(string dir, const ignoreList &_ignore_list)
        : ignore_list(_ignore_list)
    {
        tsDbDir = dir;
        string tsDb = dir + "/" + TSDB_FILENAME;
        char * key_var = getenv("TREESCAN_DBKEY");
        if (key_var != NULL)
            tsDb += string(":") + string(key_var);
        bt = Btree::openFile(tsDb.c_str(), treescan_cache_size);
        if (bt == NULL)
        {
            bt = Btree::createFile(tsDb.c_str(), treescan_cache_size,
                                   0600, treescan_bt_order);
        }
        if (bt == NULL)
        {
            cout << "unable to open " << tsDb << endl;
            exit(1);
        }
    }
    ~treescan_tree(void) {
        if (bt)
        {
            bt->get_fbi()->compact(&treescan_tree::compactionStatus,this);
            delete bt;
        }
    }
private:
    class deleteDetector : public BtreeIterator {
        pxfe_timeval now;
        Btree * bt;
        tsdatum *dat;
        /*virtual*/ bool handle_item(uint8_t *keydata, uint32_t keylen,
                                     FB_AUID_T data_fbn) {
            pxfe_timeval ls;
            if (dat == NULL)
                dat = new tsdatum(bt);
            if (dat->key.bst_decode(keydata,keylen) == false)
            {
                cout << "bst_decode failed\n";
                goto out;
            }
            if (dat->key.which() != tskey::FNAME_TO_INFO)
            {
                //cout << "not an FNAME_TO_INFO record\n";
                goto out;
            }
            if (dat->get_data(data_fbn) == false)
            {
                cout << "get data failed\n";
                goto out;
            }
            dat->data.fname_to_info.lastScanned.get(ls);
            if (ls != now)
            {
                leftovers.push_back(dat);
                dat = NULL;
            }
        out:
            if (dat != NULL) {
                dat->key.bst_free();
                dat->data.bst_free();
            }
            return true;
        }
        /*virtual*/ void print(const char *format, ...)
            __attribute__ ((format( printf, 2, 3 ))) {
        }
    public:
        deleteDetector(pxfe_timeval _now, Btree *_bt)
            : now(_now), bt(_bt) { dat = NULL; }
        ~deleteDetector(void) {
            if (dat) delete dat;
            while (leftovers.size() > 0)
            {
                delete leftovers.front();
                cout << "you left something on the leftovers list\n";
                leftovers.pop_front();
            }
        }
        list<tsdatum*> leftovers;
    };
    static void calc_sha1_hash(const string &fname, string &hash) {
        uint8_t buffer[65536];
        int  len, fd;
        mbedtls_sha1_context ctx;
        mbedtls_sha1_init( &ctx );
        MBEDTLS_SHA1_STARTS( &ctx );
        fd = ::open(fname.c_str(), O_RDONLY);
        if (fd < 0)
            goto out;
        while (1)
        {
            len = read(fd, buffer, sizeof(buffer));
            if (len <= 0)
                break;
            MBEDTLS_SHA1_UPDATE(&ctx, buffer, len);
        }
        close(fd);
    out:
        hash.resize(SHA1HashSize);
        MBEDTLS_SHA1_FINISH(&ctx, (uint8_t*) hash.c_str());
        mbedtls_sha1_free( &ctx );
    }
    static std::string format_hash(const std::string &str) {
        char buf[SHA1HashSize*2+1];
        for (int ind = 0; ind < SHA1HashSize; ind++)
        {
            unsigned char c = (unsigned char) str[ind];
            sprintf(buf + ind*2, "%02x", c);
        }
        buf[sizeof(buf)-1] = 0;
        return std::string(buf);
    }
public:
    void scan(void) {
        pxfe_timeval now;
        now.getNow();
        // cd to root of db dir, but guarantee that
        // no matter how we get out of this function,
        // we cd back to the old dir.
        class dirpusher {
            char * oldwd;
        public:
            dirpusher(const string &newdir) {
                oldwd = getcwd(NULL,0);
                if (chdir(newdir.c_str()) < 0)
                    fprintf(stderr, "cannot chdir to %s: %s\n",
                            newdir.c_str(), strerror(errno));
            }
            ~dirpusher(void) {
                if (chdir(oldwd) < 0)
                    fprintf(stderr, "cannot chdir to %s: %s\n",
                            oldwd, strerror(errno));
                free(oldwd);
            }
        } dirpusher(tsDbDir);
        {
            tsdatum dat(bt);
            dat.key.which() = tskey::DBINFO;
            dat.key.dbinfo.set("DB_INFO");
            if (dat.get() == false)
            {
                cout << "# new database" << endl;
                dat.data.which() = tsdata::DBINFO;
                lastScan.getNow();
                dat.data.dbinfo.last_scan.set(lastScan);
                dat.mark_dirty(); // put happens on destruction of dat
            }
            else
            {
                dat.data.dbinfo.last_scan.get(lastScan);
                cout << "# last scan: " 
                     << lastScan.Format() << endl;
            }
            cout << "#       now: " << now.Format() << endl;
        } // dat deleted here
        list<fileInfo> todo;
        todo.push_back(fileInfo("."));
        if (todo.back().err)
            return; // no point in descending
        map<Sha1Hash,string> newFiles;
        while (todo.size() > 0)
        {
            const fileInfo &item = todo.front();
            if (item.islnk())
            {
                // cout << "skipping link\n";
            }
            else if (item.isdir())
            {
                DIR * d = opendir(item.name.c_str());
                struct dirent * de;
                while ((de = readdir(d)) != NULL)
                {
                    string dename(de->d_name);
                    if (dename == "." || dename == "..")
                        continue; // skip
                    string newname = item.name + "/" + dename;
                    if (!ignore_file(newname))
                    {
                        todo.push_back(fileInfo(newname));
                        if (todo.back().err)
                            // take it off again
                            todo.pop_back();
                    }
                }
                closedir(d);
            }
            else if (item.name == "./" TSDB_FILENAME)
            {
                // cout << "skipping tsdb\n";
            }
            else
            {
                tsdatum dat(bt);
                dat.key.which() = tskey::FNAME_TO_INFO;
                dat.key.fname_to_info.fname.set(item.name);
                if (dat.get() == false)
                {
                    dat.data.which() = tsdata::FNAME_TO_INFO;
                    dat.data.fname_to_info.lastScanned.set(now);
                    dat.data.fname_to_info.mtime.set(item.mtime);
                    string &hash = dat.data.fname_to_info.sha1hash();
                    calc_sha1_hash(item.name, hash);
                    dat.mark_dirty();
                    cout << "N " << format_hash(hash)
                         << " " << item.name << endl;
                    Sha1Hash h;
                    memcpy(h.hash, hash.c_str(), Sha1Hash::size);
                    newFiles[h] = item.name;
                }
                else
                {
                    pxfe_timeval mt;
                    dat.data.fname_to_info.mtime.get(mt);
                    if (mt != item.mtime)
                    {
                        dat.data.fname_to_info.mtime.set(item.mtime);
                        string &hash = dat.data.fname_to_info.sha1hash();
                        string oldhash(hash);
                        calc_sha1_hash(item.name, hash);
                        if (oldhash == hash)
                            cout << "T " << item.name << endl;
                        else
                            cout << "U " << format_hash(hash)
                                 << " " << format_hash(oldhash)
                                 << " " << item.name << endl;
                    }
                    dat.data.fname_to_info.lastScanned.set(now);
                    dat.mark_dirty();
                }
            }
            todo.pop_front();
        }
        deleteDetector myIter(now,bt);
        bt->iterate(&myIter);
        while (myIter.leftovers.size() > 0)
        {
            tsdatum * dat = myIter.leftovers.front();
            myIter.leftovers.pop_front();
            const string &oldFname =
                dat->key.fname_to_info.fname();
            dat->del();
            Sha1Hash h;
            memcpy(h.hash,
                   dat->data.fname_to_info.sha1hash().c_str(),
                   Sha1Hash::size);
            map<Sha1Hash,string>::iterator  it = newFiles.find(h);
            if (it == newFiles.end())
                cout << "D " << oldFname << endl;
            else
                cout << "M " << oldFname << " " << it->second << endl;
            delete dat;
        }
        lastScan = now;
        {
            tsdatum dat(bt);
            dat.key.which() = tskey::DBINFO;
            dat.key.dbinfo.set("DB_INFO");
            dat.data.which() = tsdata::DBINFO;
            dat.data.dbinfo.last_scan.set(now);
            dat.mark_dirty();
        } // put happens on destruction of dat
    }
private:
    class dbPrinter : public BtreeIterator {
        Btree * bt;
        /*virtual*/ bool handle_item(uint8_t *keydata, uint32_t keylen,
                                     FB_AUID_T data_fbn) {
            tsdatum dat(bt);
            if (dat.key.bst_decode(keydata,keylen) == false)
                return true;
            if (dat.key.which() != tskey::FNAME_TO_INFO)
                return true;
            if (dat.get_data(data_fbn) == false)
                return true;
            cout << "N "
                 << format_hash(dat.data.fname_to_info.sha1hash())
                 << " "
                 << dat.key.fname_to_info.fname()
                 << endl;
            return true;
        }
        /*virtual*/ void print(const char *format, ...)
            __attribute__ ((format( printf, 2, 3 ))) {
#if 0
            va_list ap;
            va_start(ap,format);
            vfprintf(stdout,format,ap);
            va_end(ap);
#endif
        }
    public:
        dbPrinter(Btree * _bt) : bt(_bt) { }
        ~dbPrinter(void) { }
    };
public:
    void print(void) {
        dbPrinter  prt(bt);
        bt->iterate(&prt);
    }
private:
    Btree * bt;
    string tsDbDir;
    static const int treescan_cache_size = 256 * 1024 * 1024;
    static const int treescan_bt_order = 25;
    static bool compactionStatus(FileBlockStats *stats, void *arg) {
        if (stats->num_aus < 1000)
            // don't bother compacting a small file.
            return false;
        uint32_t max_free = stats->num_aus / 100;
        if (stats->free_aus <= max_free)
            return false;
        return true;
    }
};

void
treescan_usage(void)
{
    cout << 
"treescan args:\n"
"     -I <name> : ignore anything named <name>\n"
"     -p : dont scan any files, just print what's in the database\n";
    exit(1);
}

extern "C" int
treescan_main(int argc, char ** argv)
{
    if (argc < 2)
        treescan_usage();

    string rootdir("NOTSET");
    bool print = false;
    ignoreList  ignore_list;

    for (int ind = 1; ind < argc; ind++)
    {
        string arg(argv[ind]);
        if (arg[0] != '-')
        {
            rootdir = arg;
            if (ind != (argc - 1))
                treescan_usage();
            break;
        }
        if (arg == "-p")
            print = true;
        else if (arg == "-I")
        {
            ind++;
            ignore_list.push_back(argv[ind]);
        }
        else
            treescan_usage();
    }

    if (rootdir == "NOTSET")
        treescan_usage();

    treescan_tree  tst(rootdir, ignore_list);

    if (print)
        tst.print();
    else
        tst.scan();

    return 0;
}

#if 0
// treesync args:
//     TBD

extern "C" int
treesync_main(int argc, char ** argv)
{
}
#endif
