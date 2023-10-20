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
