
#include <iostream>
#include "screen.h"
#include "options.h"
#include "pidlist.h"
#include "pfkposix.h"

using namespace pfktop;
using namespace std;

extern "C" int
pfktop_main(int argc, char ** argv)
{
    Options opts(argc, argv);

    if (!opts.ok())
        // assume Options printed something nasty already, like usage.
        return 1;

    Screen screen;
    PidList  list(opts, screen);
    pfk_select sel;
    pfk_ticker  ticker;

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
            if (sel.rfds.isset(ticker.fd()))
            {
                int cc = read(ticker.fd(), &c, 1);
                if (cc <= 0)
                    done = true;
                list.fetch();
            }
            if (sel.rfds.isset(0))
            {
                if (read(0,&c,1) == 1)
                {
                    switch (c)
                    {
                    case 'q':
                        screen.stop_winch();
                        // 0 read from winchfd will stop process
                        break;
                    default:
                        /*nothing*/;
                    }
                }
            }
            if (sel.rfds.isset(winchfd))
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