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

// wsProxyServer destHost=blade.phillipknaack.com destPort=22
//   wsPort=1081 wsPath=/websocket/levproto

#include "WebAppServer.h"
#include "proxyServerConn.h"
#include <stdlib.h>
#include <signal.h>

using namespace std;

int usage(void)
{
    cerr << "usage: wsProxyServer destHost=<host> destPort=<port> \n"
         << "       wsPort=<port> wsPath=<path> \n";
    return 1;
}

int
main(int argc, char ** argv)
{
    signal(SIGPIPE, SIG_IGN);

    string destHost = "";
    int destPort = -1;
    int wsPort = -1;
    string wsPath = "";

    for (int argN = 1; argN < argc; argN++)
    {
        string arg(argv[argN]);
        if (arg.compare(0,9,"destHost=") == 0)
            destHost = arg.substr(9);
        else if (arg.compare(0,9,"destPort=") == 0)
            destPort = atoi(arg.substr(9).c_str());
        else if (arg.compare(0,7,"wsPort=") == 0)
            wsPort = atoi(arg.substr(7).c_str());
        else if (arg.compare(0,7,"wsPath=") == 0)
            wsPath = arg.substr(7);
        else
            return usage();
    }

    if (destHost == "" || destPort <= 0 || wsPort <= 0 || wsPath == "")
        return usage();

    WebAppServer::WebAppServerConfig  serverConfig;
    proxyServerConnCB   proxyCB(destHost, destPort);
    WebAppServer::WebAppServerServer        server;

    serverConfig.addWebsocket(wsPort, wsPath,
                              &proxyCB, /*poll*/ -1);

    server.start(&serverConfig);

    while (1)
        sleep(1);

    return 0;
}
