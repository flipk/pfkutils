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

// TODO there's some bytes getting lost somewhere. 
//      every once in a while i get an ssh error that has
//      to be a truncated packet or something. (missing HMAC)
// UPDATE actually i think this bug got fixed, but i'll leave this todo
//        here for a while just in case.

// wsProxyClient proxyConn proxy=wwwgate0.mot.com:1080
//   url=ws://leviathan.phillipknaack.com/websocket/levproto listenPort=2222

#include "proxyClientConn.h"
#include "proxyClientTcpAcceptor.h"
#include <signal.h>

using namespace std;
using namespace WebAppServer;
using namespace WebAppClient;

int usage(void)
{
    cerr << "usage: wsProxyClient [proxyConn] [proxy=<host>:<port>]\n"
         << "              url=ws://<host>[:<port>]/websocket/<wsname> \n"
         << "              listenPort=<portnumber>\n";
    return 1;
}

int
main(int argc, char ** argv)
{
    signal(SIGPIPE, SIG_IGN);

    int listenPort = -1;
    bool proxyConnect = false;
    string proxy = "";
    string url = "";

    for (int argN = 1; argN < argc; argN++)
    {
        string arg(argv[argN]);
        if (arg == "proxyConn")
            proxyConnect = true;
        else if (arg.compare(0,6,"proxy=") == 0)
            proxy = arg.substr(6);
        else if (arg.compare(0,4,"url=") == 0)
            url = arg.substr(4);
        else if (arg.compare(0,11,"listenPort=") == 0)
            listenPort = atoi(arg.substr(11).c_str());
        else
            return usage();
    }

    if (listenPort <= 0  ||  url == "")
        return usage();

    fd_mgr   mgr(false);

    mgr.register_fd(new proxyClientTcpAcceptor(listenPort, proxy,
                                               proxyConnect, url));
    while (1)
    {
        struct timeval tv = { 0, 100000 };
        try {
            mgr.loop(&tv);
        }
        catch (WSClientError x)
        {
            std::cerr << "caught WSClientError: "
                      << x.Format() << std::endl;
        }
    }
    return 0;
}
