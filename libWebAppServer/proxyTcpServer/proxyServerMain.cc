
// wsProxyServer destHost=blade.phillipknaack.com destPort=22 \
//   wsPort=1081 wsPath=/websocket/levproto

#include "WebAppServer.h"
#include "proxyServerConn.h"
#include <stdlib.h>

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
