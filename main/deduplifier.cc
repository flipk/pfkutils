
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
    mbedtls_md5_starts( &ctx );

    buffer.resize(16384);
    while (1)
    {
        cc = read(fd, (void*) buffer.c_str(), 16384);
        if (cc <= 0)
            break;
        mbedtls_md5_update( &ctx, (unsigned char*) buffer.c_str(), cc );
    }

    mbedtls_md5_finish( &ctx, (unsigned char *) digest );
    mbedtls_md5_free( &ctx );
    return true;
}


extern "C" int
deduplifier_main(int argc, char ** argv)
{
    digestFileMap_t digestFileMap;
    pendingDirs_t pendingDirs;

    if (argc != 2)
    {
        cerr << "usage : dedup /full/path/to/rootdir" << endl;
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
