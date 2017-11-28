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

#include <sys/types.h>
#include <regex.h>
#include <stdio.h>
#include <string>
#include <iostream>

using namespace std;
#if 1 // url pattern testing

// group 2 is the hostname
// group 4 is the ip address
// group 7 is the port number
// group 8 is the resource path

static const char * patt =
"^ws://((([a-zA-Z][a-zA-Z0-9]*\\.)+[a-zA-Z0-9]+)|([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+))((:([0-9]+))?)(/[a-zA-Z0-9/]+)$";

#else // proxy pattern testing

// group 2 is the hostname
// group 4 is the ip address
// group 7 is the port number

static const char * patt =
"^((([a-zA-Z][a-zA-Z0-9]*\\.)+[a-zA-Z0-9]+)|([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+))((:([0-9]+))?)$";

#endif // proxy pattern testing

int
main(int argc, char ** argv)
{
    regex_t  reg;
    regmatch_t   matches[10];

    if (argc != 2)
        return 1;

    std::string  urlStr(argv[1]);

    int e = regcomp(&reg, patt, REG_EXTENDED);
    if (e != 0)
    {
        printf("regcomp returned %d\n", e);
        return 1;
    }

    e = regexec(&reg, urlStr.c_str(), 10, matches, 0);
    if (e != 0)
    {
        printf("regexec returned %d\n", e);
        return 1;
    }

#define MATCH(n) (matches[(n)].rm_so != -1)
#define MATCHSTR(str,n) str.substr( matches[(n)].rm_so, \
                  matches[(n)].rm_eo - matches[(n)].rm_so)

    if (MATCH(2))
        cout << "hostname : " << MATCHSTR(urlStr,2) << endl;
    if (MATCH(4))
        cout << "ipaddr : " << MATCHSTR(urlStr,4) << endl;
    if (MATCH(7))
        cout << "port : " << MATCHSTR(urlStr,7) << endl;
    if (MATCH(8))
        cout << "path : " << MATCHSTR(urlStr,8) << endl;

    regfree(&reg);
    return 0;
}
