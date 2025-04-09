
#include <unistd.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <list>
#include <map>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <fnmatch.h>
#include <mbedtls/sha256.h>
#include "pfkutils_config.h"

static bool
calc_hash(const std::string &path, unsigned char output[32])
{

    // we only take the hash over this much of a file,
    // because that's generally enough to be unique anyway.
    static const int temp_buf_size = 0x20000;
    static unsigned char temp_buffer[temp_buf_size];

    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0)
    {
        int e = errno;
        printf("unable to open '%s': %s\n",
               path.c_str(), strerror(e));
        return false;
    }

    int cc = read(fd, (void*) temp_buffer, temp_buf_size);
    if (cc < 0)
    {
        int e = errno;
        close(fd);
        printf("unable to read '%s': %s\n",
               path.c_str(), strerror(e));
        return false;
    }
    close(fd);

    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    MBEDTLS_SHA256_STARTS(&ctx, /*is224*/0);
    MBEDTLS_SHA256_UPDATE(&ctx, temp_buffer, cc);
    MBEDTLS_SHA256_FINISH(&ctx, output);
    mbedtls_sha256_free(&ctx);

    return true;
}

struct file_list_item
{
    const std::string relpath;
    uint64_t size;
    uint64_t ino;
    const struct timespec mtime;
    unsigned char hash[32];
    file_list_item(const std::string &_relpath,
                   uint64_t _size, uint64_t _ino,
                   const struct timespec &_mtime)
        : relpath(_relpath), size(_size), ino(_ino), mtime(_mtime)
    {
        memset(hash,0,32);
    }
    file_list_item(const std::string &_relpath,
                   uint64_t _size, uint64_t _ino,
                   const struct timespec &_mtime,
                   const unsigned char _hash[32])
        : relpath(_relpath), size(_size), ino(_ino), mtime(_mtime)
    {
        memcpy(hash,_hash,32);
    }
    void update_hash(const std::string &rootdir)
    {
        const std::string path = rootdir + "/" + relpath;
        if (calc_hash(path.c_str(), hash) == false)
        {
            memset(hash, 0, sizeof(hash));
        }
    }
};

struct file_list_db
{
    typedef std::map<std::string,file_list_item*> item_name_map_t;
    typedef std::map<uint64_t,file_list_item*> item_ino_map_t;
    item_name_map_t items_by_name;
    item_ino_map_t  items_by_ino;

    file_list_db(void)
    {
    }
    ~file_list_db(void)
    {
        item_name_map_t::iterator it;
        // the maps will take care of themselves, but the
        // pointers they contain do not. release that memory.
        for (it  = items_by_name.begin();
             it != items_by_name.end();    it++)
            delete it->second;
    }    
    void add_file(const std::string &relpath,
                  uint64_t size, uint64_t ino,
                  struct timespec mtime)
    {
        unsigned char hash[32];
        memset(hash,0,sizeof(hash));
        add_file(relpath,size,ino,mtime,hash);
    }
    void add_file(const std::string &relpath,
                  uint64_t size, uint64_t ino,
                  struct timespec &mtime,
                  const unsigned char hash[32])
    {
        file_list_item * i = new file_list_item(relpath, size,
                                                ino, mtime, hash);
        items_by_name.insert(item_name_map_t::value_type(relpath, i));
        items_by_ino.insert(item_ino_map_t::value_type(ino, i));
    }
    bool load(const std::string &fname)
    {
        std::ifstream  f(fname.c_str());
        std::string  l;

        if (!f.good())
        {
            int e = errno;
            std::cerr << "can't load : "
                      << strerror(e) << std::endl;
            return false;
        }

        while (1)
        {
            std::getline(f, l);
            if (!f.good())
                break;

            size_t first_colon = l.find(" : ");
            if (first_colon == std::string::npos)
            {
                printf("first failure\n");
                return false;
            }

            size_t second_colon = l.find(" : ", first_colon+1);
            if (second_colon == std::string::npos)
            {
                printf("second failure\n");
                return false;
            }

            size_t third_colon = l.find(" : ", second_colon+1);
            if (third_colon == std::string::npos)
            {
                printf("third failure\n");
                return false;
            }

            size_t fourth_colon = l.find(" : ", third_colon+1);
            if (fourth_colon == std::string::npos)
            {
                printf("fourth failure\n");
                return false;
            }

            size_t fifth_colon = l.find(" : ", fourth_colon+1);
            if (fifth_colon == std::string::npos)
            {
                printf("fifth failure\n");
                return false;
            }

            unsigned char hash[32];
            char byte[3];
            const char *in = l.c_str() + fifth_colon + 3;

            for (int ind = 0; ind < 32; ind++)
            {
                byte[0] = *in++;
                byte[1] = *in++;
                byte[2] = 0;
                unsigned int v = strtoul(byte, NULL, 16);
                hash[ind] = (unsigned char) v;
            }

            size_t sz = 0;

            if (0)
            {
            std::cout << "fname : '"
                      << l.substr(0,first_colon)
                      << "'" << std::endl;
            std::cout << "size : '"
                      << l.substr(first_colon + 3,
                                  second_colon - first_colon - 3)
                      << "' : "
                      << std::stoull(
                          l.substr(first_colon + 3,
                                   second_colon - first_colon - 3),
                          &sz, 10)
                      << std::endl;
            std::cout << "ino : "
                      << l.substr(second_colon + 3,
                                  third_colon - second_colon - 3)
                      << "' : "
                      << std::stoull(
                          l.substr(second_colon + 3,
                                   third_colon - second_colon - 3),
                          &sz, 10)
                      << std::endl;
            std::cout << "tvsec : "
                      << l.substr(third_colon + 3,
                                  fourth_colon - third_colon - 3)
                      << "' : "
                      << std::stoull(
                          l.substr(third_colon + 3,
                                   fourth_colon - third_colon - 3),
                          &sz, 10)
                      << std::endl;
            std::cout << "tvnsec : "
                      << l.substr(fourth_colon + 3,
                                  fifth_colon - fourth_colon - 3)
                      << "' : "
                      << std::stoull(
                          l.substr(fourth_colon + 3,
                                   fifth_colon - fourth_colon - 3),
                          &sz, 10)
                      << std::endl;
            std::cout << "hash : "
                      << l.substr(fifth_colon + 3)
                      << std::endl << "hash : ";

            for (size_t ind = 0; ind < 32; ind++)
                std::cout << std::hex << std::setw(2) << std::setfill('0') <<
                    (int)(hash[ind]);

            std::cout << std::dec << std::endl;

            }

            struct timespec mtime;
            mtime.tv_sec = std::stoull(
                          l.substr(third_colon + 3,
                                   fourth_colon - third_colon - 3),
                          &sz, 10);
            mtime.tv_nsec = std::stoull(
                          l.substr(fourth_colon + 3,
                                   fifth_colon - fourth_colon - 3),
                          &sz, 10);

            add_file(l.substr(0,first_colon),
                     std::stoull(
                          l.substr(first_colon + 3,
                                   second_colon - first_colon - 3),
                          &sz, 10),
                     std::stoull(
                          l.substr(second_colon + 3,
                                   third_colon - second_colon - 3),
                          &sz, 10),
                     mtime, hash);
        }

        return true;
    }
    bool save(const std::string &fname)
    {
        std::ofstream  f(fname.c_str(),
                         std::ios_base::out | std::ios_base::trunc);

        if (!f.good())
        {
            int e = errno;
            std::cerr << "FAIL : can't save : "
                      << strerror(e) << std::endl;
            return false;
        }

        item_name_map_t::iterator it;
        for (it = items_by_name.begin();
             it != items_by_name.end();   it++)
        {
            file_list_item * i = it->second;
            f << std::dec
              << i->relpath << " : "
              << i->size << " : "
              << i->ino << " : "
              << i->mtime.tv_sec << " : "
              << i->mtime.tv_nsec << " : ";
            for (size_t ind = 0; ind < 32; ind++)
                f << std::hex << std::setw(2) << std::setfill('0') <<
                    (int)(i->hash[ind]);
            f << std::endl;
        }

        return true;
    }
    void print(void)
    {
        item_name_map_t::iterator it;
        for (it = items_by_name.begin();
             it != items_by_name.end();   it++)
        {
            file_list_item * i = it->second;
            printf("%s : %lu : %lu : %lu : %lu : ",
                   i->relpath.c_str(),
                   (uint64_t) i->size,
                   (uint64_t) i->ino,
                   (uint64_t) i->mtime.tv_sec,
                   (uint64_t) i->mtime.tv_nsec);
            for (int ind = 0; ind < 32; ind++)
                printf("%02x", i->hash[ind]);
            printf("\n");
        }
    }
};

typedef std::list<std::string>  ignore_patterns_t;
ignore_patterns_t ignore_patterns;

static bool
is_ignored(const std::string &n)
{
    ignore_patterns_t::iterator it;

    for (it = ignore_patterns.begin();
         it != ignore_patterns.end();
         it++)
    {
        if (fnmatch(it->c_str(), n.c_str(),
                    FNM_EXTMATCH) == 0)
        {
            return true;
        }
    }

    return false;
}

static void
scandir(file_list_db &db,
        const std::string &rootpath)
{
    std::list<std::string> todo;

    todo.push_back(rootpath);

    while (todo.size() > 0)
    {
        std::string p = todo.front();
        todo.pop_front();
        DIR * d = opendir(p.c_str());
        if (d)
        {
            struct dirent * de;
            while ((de = readdir(d)) != NULL)
            {
                struct stat sb;
                std::string n = de->d_name;
                if (n == "." || n == "..")
                    continue;
                if (is_ignored(n))
                    continue;
                std::string e = p + '/' + n;
                if (lstat(e.c_str(), &sb) != 0)
                {
                    int e = errno;
                    char * err = strerror(errno);
                    printf("stat: %s\n", err);
                }
                else
                {
                    if (S_ISDIR(sb.st_mode))
                    {
                        todo.push_back(e);
                    }
                    else if (S_ISREG(sb.st_mode))
                    {
                        db.add_file(e.substr(rootpath.size() + 1),
                                    sb.st_size, sb.st_ino,
                                    sb.st_mtim);
                    }
                }
            }
            closedir(d);
        }
    }
}

static void
compare_dbs(const std::string &rootdir,
            file_list_db &old_db,
            file_list_db &new_db)
{
    file_list_db::item_ino_map_t::iterator  old_ino_it;
    file_list_db::item_ino_map_t::iterator  new_ino_it;

    // go through every inode in old, see if it's in new.
    // (detect deletions, and modifications)

    for (old_ino_it = old_db.items_by_ino.begin();
         old_ino_it != old_db.items_by_ino.end();
         old_ino_it ++)
    {
        file_list_item * old_item = old_ino_it->second;
        new_ino_it = new_db.items_by_ino.find(old_item->ino);
        if (new_ino_it != new_db.items_by_ino.end())
        {
            file_list_item * new_item = new_ino_it->second;

            // if name is different, it has been renamed.
            if (old_item->relpath != new_item->relpath)
            {
                printf("M \"%s\" \"%s\"\n",
                       old_item->relpath.c_str(),
                       new_item->relpath.c_str());
            }

            if (old_item->size != new_item->size ||
                old_item->mtime.tv_sec != new_item->mtime.tv_sec ||
                old_item->mtime.tv_nsec != new_item->mtime.tv_nsec)
            {
                printf("U \"%s\"\n", new_item->relpath.c_str());
                new_item->update_hash(rootdir);
            }
            else
                memcpy(new_item->hash, old_item->hash, 32);
        }
        else
        {
            // if in old but not new, it was deleted
            printf("D \"%s\"\n", old_item->relpath.c_str());
        }
    }

    // go through every inode in new, see if it's in old.
    // (detect new files)

    for (new_ino_it  = new_db.items_by_ino.begin();
         new_ino_it != new_db.items_by_ino.end();
         new_ino_it ++)
    {
        file_list_item * new_item = new_ino_it->second;
        old_ino_it = old_db.items_by_ino.find(new_item->ino);
        if (old_ino_it == old_db.items_by_ino.end())
        {
            printf("N \"%s\"\n", new_item->relpath.c_str());
            new_item->update_hash(rootdir);
        }
    }
}

static void
usage(void)
{
    printf("usage: ts2 -d <dir> -I <pattern> [-p]\n"
           "  -d <dir> : the directory to scan\n"
           "  -I <pattern> : ignore any file or directory matching glob\n"
           "  -p : don't scan, just print what's in the database\n");
}

extern "C" int
ts2_main(int argc, char **argv)
{
    std::string rootdir;
    bool justprint = false;

    for (int i = 1; i < argc; i++)
    {
        std::string opt = argv[i];
        if (opt == "-d")
        {
            rootdir = argv[i+1];
            i++;
        }
        else if (opt == "-I")
        {
            ignore_patterns.push_back(argv[i+1]);
            i++;
        }
        else if (opt == "-p")
        {
            justprint = true;
            break;
        }
    }

    file_list_db old_db, new_db;

    if (rootdir == "")
    {
        printf("error must set rootdir\n");
        usage();
        return 1;
    }

    std::string rootdir_db = rootdir + "/" + ".ts2.db";

    if (justprint)
    {
        if (old_db.load(rootdir_db) == false)
        {
            printf("error cannot load db\n");
            return 1;
        }
        old_db.print();
        return 0;
    }

    ignore_patterns.push_back(".tsdb");
    ignore_patterns.push_back(".ts2.db");

    old_db.load(rootdir_db);
    scandir(new_db, rootdir);
    // note compare_dbs will update hashes for anything touched,
    // so dont save until after compare.
    compare_dbs(rootdir, old_db, new_db);
    new_db.save(rootdir_db);

    return 0;
}
