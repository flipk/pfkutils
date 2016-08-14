
#include "fileparser.h"
#include <fstream>

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

    // there might be fields with parens, and those fields
    // might have spaces. delete the parens, but don't forget
    // the spaces inside the parens aren't field delimiters.
    // also: there might be double parens ((like this)).
    // also: linux /proc files don't do this but just for
    // completeness we should handle the last field having parens.

    size_t pos = 0, spacepos;
    while (1)
    {
        if (line[pos] == '(')
        {
            size_t firstcharpos = line.find_first_not_of('(',pos);
            if (firstcharpos == string::npos)
                // buh? this line sucks.
                return -1;
            size_t parenpos = line.find_first_of(')',firstcharpos);
            if (parenpos == string::npos)
                // this line sucks some other way.
                return -1;
            spacepos = line.find_first_of(' ',parenpos+1);
            fields.push_back(line.substr(firstcharpos,
                                         parenpos-firstcharpos));
        }
        else
        {
            spacepos = line.find_first_of(' ',pos);
            if (spacepos == string::npos)
                fields.push_back(line.substr(pos));
            else
                fields.push_back(line.substr(pos,spacepos-pos));
        }
        if (spacepos == string::npos)
            break;
        pos = spacepos+1;
    }

    return numFields();
}
