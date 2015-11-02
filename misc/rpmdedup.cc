
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <stdio.h>
#include <sys/types.h>
#include <regex.h>

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <algorithm>

using namespace std;

const char *rpmpattern_string =
    "^([a-z][_+a-z0-9-]+)-(.*)(\\.|_)(x86_64|i686|noarch)\\.rpm$";
#define NAME_PATTERN_INDEX 1
#define VERSION_PATTERN_INDEX 2

#define PATTERN_STR(str,match,index)\
    str.substr(match[index].rm_so,match[index].rm_eo-match[index].rm_so)

struct rpm_file_info {
    string version;
    string fname;
    bool operator<(const rpm_file_info &other) const {
        return (version < other.version);
    }
};

typedef vector<rpm_file_info> rpm_file_info_list;

struct rpm_package {
    string name;
    rpm_file_info_list  files;
};

typedef map<string,rpm_package> packageMap;

int
main()
{
    DIR * d = opendir(".");
    struct dirent *de;
    regex_t  rpmpattern;
    int regcode;
#define NUM_MATCHES 16
    regmatch_t  regmatches[NUM_MATCHES];
    packageMap  packages;

    regcode = regcomp(&rpmpattern, rpmpattern_string,
                      REG_EXTENDED | REG_ICASE);
    if (regcode != 0)
    {
        printf("regcomp returned err %d\n", regcode);
        return 1;
    }

    if (d == NULL)
    {
        printf("opendir: %s\n", strerror(errno));
        return 1;
    }

    while ((de = readdir(d)) != NULL)
    {
        string  fname(de->d_name);
        if (fname == "." || fname == "..")
            continue;

        if (fname.size() > 4 &&
            fname.compare(fname.size() - 4, 4, ".rpm") == 0)
        {
#if 0 // debug
            cout << fname << endl;
#endif
            regcode = regexec(&rpmpattern, fname.c_str(),
                              NUM_MATCHES, regmatches, 0);
            if (regcode != 0)
            {
                printf("*** regexec returns %d\n", regcode);
            }
            else
            {
#if 0 // debug prints
                int ind;
                for (ind = 0; ind < NUM_MATCHES; ind++)
                {
                    if (regmatches[ind].rm_so == -1)
                        continue;
                    string sub = PATTERN_STR(fname,regmatches,ind);
                    printf("%d(%d-%d):'%s'\n", ind,
                           regmatches[ind].rm_so,
                           regmatches[ind].rm_eo, sub.c_str() );
                }
#endif

                string packageName = PATTERN_STR(fname,regmatches,
                                                 NAME_PATTERN_INDEX);
                string versionInfo = PATTERN_STR(fname,regmatches,
                                                 VERSION_PATTERN_INDEX);

                packageMap::iterator it = packages.find(packageName);
                rpm_file_info  rfi;
                rfi.version = versionInfo;
                rfi.fname = fname;
                if (it == packages.end())
                {
                    rpm_package rp;
                    rp.name = packageName;
                    rp.files.push_back(rfi);
                    packages[packageName] = rp;
                }
                else
                {
                    rpm_package &rp = it->second;
                    rp.files.push_back(rfi);
                }
            }
        }
    }

    closedir(d);

    packageMap::iterator it;
    for (it = packages.begin(); it != packages.end(); it++)
    {
        rpm_package &rp = it->second;
        int n = rp.files.size();
        if (n > 1)
        {
            int cnt = 0;
            stable_sort(rp.files.begin(), rp.files.end());
            rpm_file_info_list::iterator  it2;
            for (it2 = rp.files.begin(); it2 != rp.files.end(); it2++)
            {
                rpm_file_info &rfi = *it2;
                if (cnt++ == (n-1))
                    break;
                printf("mv %s no/\n",
                       rfi.fname.c_str());
            }
        }
    }

    return 0;
}
