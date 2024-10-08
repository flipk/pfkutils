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
#include "screen.h"
#include "options.h"
#include "pidlist.h"
#include "posix_fe.h"

using namespace pfktop;
using namespace std;

extern "C" int
pfktop_main(int argc, char ** argv)
{
    Screen screen;
    Options opts(argc, argv, screen);

    if (!opts.ok())
        // assume Options printed something nasty already, like usage.
        return 1;

    PidList  list(opts, screen);
    pxfe_select sel;
    pxfe_ticker  ticker;

    bool done = false;

    cout << screen.home << screen.erase;

    list.fetch();
    int winchfd = screen.start_winch();
    ticker.start(1,0);
    while (!done)
    {
        char c;

        list.print();
        cout.flush();

        sel.rfds.zero();
        sel.rfds.set(0);
        sel.rfds.set(winchfd);
        sel.rfds.set(ticker.fd());
        sel.tv.set(1,0);
        if (sel.select() > 0)
        {
            if (sel.rfds.is_set(ticker.fd()))
            {
                ticker.doread();
                list.fetch();
            }
            if (sel.rfds.is_set(0))
            {
                if (read(0,&c,1) == 1)
                {
                    switch (c)
                    {
                    case 'h': // help
                        ticker.pause();
                        opts.usage(true);
                        if (read(0,&c,1) <= 0)
                        {
                            cout << "stdin read error!\n";
                        }
                        ticker.resume();
                        break;
                    case 'i': // sort by pId
                    case 'p': // sort by Prio
                    case 'v': // sort by Vsz
                    case 'r': // sort by Rss
                    case 't': // sort by Time
                    case 'c': // sort by Cmd
                    case 'R': // reverse sort order
                    case '1': // sort by 10s
                    case 'l': // light background
                    case 'd': // dark background
                        opts.set_option(c);
                        break;
                    case 'q': // quit
                        screen.stop_winch();
                        // 0 read from winchfd will stop process
                        break;
                    default:
                        /*nothing*/;
                    }
                }
            }
            if (sel.rfds.is_set(winchfd))
            {
                int cc = read(winchfd, &c, 1);
                if (cc == 1)
                    // go round again
                    continue;
                done = true;
            }
        }
    }
    ticker.stopjoin();

    cout << screen.nl << screen.nl;

    return 0;
}
