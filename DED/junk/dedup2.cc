
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <map>
#include <list>
#include <iostream>
#include <iomanip>

#include <mbedtls/sha256.h>

#include "posix_fe.h"

using namespace std;

struct SHA256HASH {
    static const int size = 32;
    unsigned char hash[size];
    bool operator==(const SHA256HASH &other)
    {
        return memcmp(hash,other.hash,size) == 0;
    }
};

static ostream& operator<<(ostream &str, const SHA256HASH &h)
{
    for (int ind = 0; ind < SHA256HASH::size; ind++)
        str << std::hex
            << std::setw(2)
            << std::setfill('0')
            << (int) h.hash[ind];
    return str;
}

struct fileInfo {
    uint64_t size;
    pxfe_timespec mtime;
    SHA256HASH hash;
};

typedef map<string/*filename*/,fileInfo> FilesInfo;

static bool
sha256file(const std::string &path, SHA256HASH &output)
{
    pxfe_fd fd;
    pxfe_errno e;

    if (!fd.open(path, O_RDONLY, &e))
    {
        cout << e.Format() << endl;
        return false;
    }

    pxfe_string temp_buffer;
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts_ret(&ctx, /*is224*/0);

    bool ret = true;

    while (1)
    {
        if (!fd.read(temp_buffer, 65536, &e))
        {
            cout << e.Format() << endl;
            ret = false;
            break;
        }
        size_t len = temp_buffer.length();
        if (len == 0)
            break;
        mbedtls_sha256_update_ret(&ctx,
                                  temp_buffer.ucptr(), len);
    }

    mbedtls_sha256_finish_ret(&ctx, output.hash);
    mbedtls_sha256_free(&ctx);

    return ret;
}

static bool
walktree(const char *first_dirpath, FilesInfo &info)
{
    pxfe_readdir d;
    pxfe_errno e;
    dirent de;
    list<string> dirs;

    dirs.push_back(first_dirpath);

    while (!dirs.empty())
    {
        string dirpath = dirs.front();
        dirs.pop_front();
            
        if (!d.open(dirpath.c_str(), &e))
        {
            printf("directory '%s': %s\n",
                   dirpath.c_str(), e.Format().c_str());
            continue;
        }

        while (d.read(de))
        {
            if (strcmp(de.d_name, ".") == 0 ||
                strcmp(de.d_name, "..") == 0)
                // skip
                continue;

            string p = dirpath + "/" + de.d_name;
            bool do_dir = false, do_file = false;
            struct stat sb;

            if (lstat(p.c_str(), &sb) < 0)
            {
                e.init(errno, "stat");
                cout << e.Format() << endl;
            }

            if (S_ISREG(sb.st_mode))
                do_file = true;
            else if (S_ISDIR(sb.st_mode))
                do_dir = true;

            if (do_file)
            {
                fileInfo &f = info[p];
                f.size = sb.st_size;
                f.mtime = sb.st_mtim;
                sha256file(p, f.hash);
            }

            if (do_dir)
                dirs.push_back(p);
        }
    }

    return true;
}

static void
printtree(FilesInfo &fi)
{
    for (auto &f : fi)
    {
        cout << f.second.mtime.Format()
             << " "
             << f.second.hash
             << " "
             << f.first // filename
             << endl;
    }
}

bool
get_current_dir(string &path)
{
    pxfe_errno e;
    path.resize(PATH_MAX);
    if (getcwd((char*) path.c_str(), path.length()) == NULL)
    {
        e.init(errno, "getcwd");
        cout << e.Format() << endl;
        path.resize(0);
        return false;
    }
    path.resize(strlen(path.c_str()));
    return true;
}

extern "C" int
dedup2_main(int argc, char ** argv)
{
    FilesInfo fi1, fi2;
    string starting_dir, first_dir, second_dir;
    pxfe_errno e;
    bool dbg = (getenv("DBG") != NULL);

    if (argc != 3)
    {
        printf("usage: dedup2 /path/to/tree1 /path/to/tree2\n");
        return 1;
    }

    if (!get_current_dir(starting_dir))
        return 1;

    if (chdir(argv[1]) < 0)
    {
        e.init(errno, "chdir 1");
        cout << e.Format() << endl;
        return 1;
    }
    get_current_dir(first_dir);
    if (!walktree(".", fi1))
        return 1;
    if (dbg)
    {
        cout << "----------TREE 1------------" << endl;
        printtree(fi1);
    }

    if (chdir(starting_dir.c_str()) < 0)
    {
        e.init(errno, "chdir 2");
        cout << e.Format() << endl;
        return 1;
    }
    if (chdir(argv[2]) < 0)
    {
        e.init(errno, "chdir 3");
        cout << e.Format() << endl;
        return 1;
    }
    get_current_dir(second_dir);
    if (!walktree(".", fi2))
        return 1;
    if (dbg)
    {
        cout << "----------TREE 2------------" << endl;
        printtree(fi2);
        cout << "----------COMMANDS------------" << endl;
    }

    for (auto &f : fi1)
    {
        FilesInfo::iterator it = fi2.find(f.first);

        if (it == fi2.end())
            continue;

        if (f.second.hash == it->second.hash)
        {
            string source_path = first_dir + "/" + f.first;
            string  dest_path = second_dir + "/" + f.first;

            cout << "rm -f " << dest_path << endl
                 << "ln " << source_path << " " << dest_path << endl;
        }
    }

    return 0;
}
