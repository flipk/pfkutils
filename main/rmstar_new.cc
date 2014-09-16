
/*
    This file is part of the "pfkutils" tools written by Phil Knaack
    (pknaack1@netscape.net).
    Copyright (C) 2008  Phillip F Knaack

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <iostream>
#include <sstream>
#include <list>
#include <string>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>

using namespace std;
static ostringstream errors;

typedef list<string> removelist;

#define ERR(s) \
    errors << #s << path << " : " << strerror(errno) << endl

// args : in: path   out: err
// return : true if path is a directory.
//          false if path is not a dir, or error.
//          note: ret = true implies err = false

static bool
isdir(const string &path, bool &err)
{
    bool ret = false;
    struct stat sb;
    if (lstat(path.c_str(), &sb) < 0)
    {
        ERR(lstat);
        err = true;
    }
    else
    {
        err = false;
        if (S_ISDIR(sb.st_mode))
            ret = true;
    }
    return ret;
}

static void
myrmfile(const string &path)
{
//    cout << "rmfile " << path << endl;
    chmod(path.c_str(), 0600);
    if (unlink(path.c_str()) < 0)
        ERR(unlink);
}

static void
myrmdir(const string &path)
{
    cout << "rmdir " << path << endl;
    if (rmdir(path.c_str()) < 0)
        ERR(rmdir);
}

static void
rmlist(removelist &l)
{
    while (l.size() > 0)
    {
        const string &path = l.front();
        bool err;
        if (isdir(path,err))
        {
            chmod(path.c_str(), 0700);
            DIR * d = opendir(path.c_str());
            if (d == NULL)
                ERR(opendir);
            else
            {
                removelist dirl;
                struct dirent * de;
                while ((de = readdir(d)) != NULL)
                {
                    string dename(de->d_name);
                    if (dename == "." || dename == "..")
                        continue;
                    dirl.push_back(path + "/" + dename);
                }
                closedir(d);
                rmlist(dirl);
                myrmdir(path);
            }
        }
        else if (!err)
            myrmfile(path);
        l.pop_front();
    }
}

extern "C" int rmstar_main( int, char ** );

int
rmstar_main(int argc, char ** argv)
{
    removelist l;
    for (int argn = 1; argn < argc; argn++)
        l.push_back(argv[argn]);
    rmlist(l);
    cerr << errors.str();
    return 0;
}
