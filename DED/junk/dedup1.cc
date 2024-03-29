
#include "pfkutils_config.h"
#include <dirent.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include <iostream>
#include <string>
#include <map>
#include <list>
#include <fcntl.h>

#include "mbedtls/md5.h"

using namespace std;

typedef map<string,string> digestFileMap_t;
typedef list<string> pendingDirs_t;

static bool
MD5File(const std::string &path, char digest[16])
{
    int cc, fd = open(path.c_str(), O_RDONLY);
    if (fd < 0)
        return false;
    mbedtls_md5_context  ctx;
    std::string buffer;

    mbedtls_md5_init( &ctx );
    MBEDTLS_MD5_STARTS( &ctx );

    buffer.resize(16384);
    while (1)
    {
        cc = read(fd, (void*) buffer.c_str(), 16384);
        if (cc <= 0)
            break;
        MBEDTLS_MD5_UPDATE( &ctx, (unsigned char*) buffer.c_str(), cc );
    }

    MBEDTLS_MD5_FINISH( &ctx, (unsigned char *) digest );
    mbedtls_md5_free( &ctx );
    return true;
}


extern "C" int
dedup1_main(int argc, char ** argv)
{
    digestFileMap_t digestFileMap;
    pendingDirs_t pendingDirs;

    if (argc != 2)
    {
        cerr << "usage : dedup1 /full/path/to/rootdir" << endl;
        return 1;
    }

    string rootdir = argv[1];

    if (rootdir[0] != '/')
    {
        cerr << "must use absolute path" << endl;
        return 1;
    }

    if (rootdir[rootdir.size()-1] == '/')
        // strip trailing slash
        rootdir.erase(rootdir.size()-1,1);

    pendingDirs.push_back(rootdir);

    while (!pendingDirs.empty())
    {
        const string &currentDir = pendingDirs.front();
        DIR * d = opendir(currentDir.c_str());
        if (d)
        {
            struct dirent * de;
            while ((de = readdir(d)) != NULL)
            {
                const string fname = de->d_name;
                if (fname == "." || fname == ".." || fname == ".git")
                    continue;
                const string fullPath = currentDir + "/" + fname;
                struct stat sb;
                if (lstat(fullPath.c_str(), &sb) == 0)
                {
                    if (S_ISDIR(sb.st_mode))
                        pendingDirs.push_back(fullPath);
                    else if (S_ISREG(sb.st_mode))
                    {
                        char buffer[16];
                        if (MD5File(fullPath, buffer))
                        {
                            string digest(buffer, 16);
                            pair<digestFileMap_t::iterator,bool> rv;
                            rv = digestFileMap.insert(
                                digestFileMap_t::value_type(
                                    digest,fullPath));
                            if (rv.second == false)
                            {
                                cout << rv.first->second
                                     << " same as " << fullPath << endl;
                                (void) unlink(fullPath.c_str());
                                if (symlink(rv.first->second.c_str(),
                                            fullPath.c_str()) < 0)
                                    cerr << "unable to symlink: "
                                         << strerror(errno) << endl;
                            }
                        }
                        else
                            cerr << "unable to md5 " << fullPath << endl;
                    }
                }
                else
                    cerr << "unable to stat " << fullPath << endl;
            }
            closedir(d);
        }
        else
            cerr << "unable to open " << currentDir << endl;
        pendingDirs.pop_front();
    }

    return 0;
}
