
#include "pidlist.h"
#include "screen.h"
#include "pfkposix.h"

#include <iostream>
#include <fstream>
#include <stdlib.h>

using namespace pfktop;
using namespace std;

fileParser :: fileParser(void)
{
}

fileParser :: ~fileParser(void)
{
}

int
fileParser :: parse(const char *fname)
{
    fields.clear();

    {
        ifstream  inf(fname);
        if (!inf.good())
            return -1;
        line.clear();
        getline(inf, line);
    } // inf closed here
//    cout << "got line : '" << line << "' ";

    size_t pos = 0, spacepos;
    while (1)
    {
        // special case : the process title may have a space
        // in it, and has parens to delimit the field. if a
        // field starts with a '(' we should search for the
        // matching ')'.
        if (line[pos] == '(')
        {
            size_t parenpos = line.find_first_of(')',pos);
            if (parenpos == string::npos)
                // WUT
                return -1;
            spacepos = line.find_first_of(' ',parenpos);
            fields.push_back(line.substr(pos+1,parenpos-pos-1));
        }
        else
        {
            spacepos = line.find_first_of(' ',pos);
            fields.push_back(line.substr(pos,spacepos-pos));
        }
        if (spacepos == string::npos)
            break;
        pos = spacepos+1;
    }

    return numFields();
}
