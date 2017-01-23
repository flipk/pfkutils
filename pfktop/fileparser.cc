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
