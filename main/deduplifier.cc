#if 0
set -e -x
g++ -I../libpfkutil ../libpfkutil/md5.c deduplifier.cc -o d
exit 0
#endif

#include <dirent.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <map>
#include <list>

#include "md5.h"

using namespace std;

typedef map<string,string> digestFileMap_t;
typedef list<string> pendingDirs_t;

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
                        char buffer[48];
                        if (MD5File(fullPath.c_str(), buffer) != NULL)
                        {
                            string digest = (string)((char*)buffer);
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
