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

#include "options.h"

#include <iostream>

using namespace std;
using namespace pfktop;

Options :: Options(int argc, const char * const * argv,
                   Screen &_screen)
    : screen(_screen)
{
    sort = SORT_10AV;
    reverse = false;
    background = BG_LIGHT;

    // ..... we could support command line options here ......

    char * modevar = getenv("PFKTOP_MODE");
    if (modevar)
    {
        if (modevar[0] == 'l')
            background = BG_LIGHT;
        else if (modevar[0] == 'd')
            background = BG_DARK;
    }

    screen.set_light_background(background == BG_LIGHT);
    isOk = true;
}

Options :: ~Options(void)
{
}

bool
Options :: set_option(char c)
{
    switch (c)
    {
    case 'i': // sort by tId
        sort = SORT_TID;
        break;
    case 'p': // sort by Prio
        sort = SORT_PRIO;
        break;
    case 'r': // sort by Rss
        sort = SORT_RSS;
        break;
    case 't': // sort by Time
        sort = SORT_TIME;
        break;
    case '1': // sort by 10s avg
        sort = SORT_10AV;
        break;
    case 'c': // sort by Cmd
        sort = SORT_CMD;
        break;
    case 'R': // reverse order
        reverse = !reverse;
        break;
    case 'l': // light background
        background = BG_LIGHT;
        screen.set_light_background(true);
        break;
    case 'd': // dark background
        background = BG_DARK;
        screen.set_light_background(false);
        break;
    default:
        return false;
    }
    return true;
}

void
Options :: usage(bool color)
{
    cout
        << screen.home
        << screen.header_color
        << "       options              -pfktop-       "
        << "                                "
        << screen.normal_color << screen.nl
        << screen.nl
        << "         q : quit" << screen.nl
        << screen.nl
        << "         i : sort by tid" << screen.nl
        << "         p : sort by prio (then by 10s avg)" << screen.nl
        << "         r : sort by rss (then by 10s avg)" << screen.nl
        << "         t : sort by time" << screen.nl
        << "         1 : sort by 10s avg of time" << screen.nl
        << "         c : sort by cmd (then by 10s avg)" << screen.nl
        << "         R : reverse sort order" << screen.nl
        << screen.nl
        << "         l : light background mode" << screen.nl
        << "         d : dark background mode" << screen.nl
        << screen.nl
        << "   ENV var PFKTOP_MODE can be set to 'l' or 'd'" << screen.nl
        << screen.nl
        << screen.header_color
        << "       options              -pfktop-       "
        << "                                "
        << screen.normal_color << screen.erase;
    cout.flush();
}
