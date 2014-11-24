
// wsProxyClient proxyConn proxy=wwwgate0.mot.com:1080 \
//   url=ws://leviathan.phillipknaack.com/websocket/levproto listenPort=2222

#include "proxyClientConn.h"
#include "proxyClientTcpAcceptor.h"

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
        mgr.loop(&tv);
    }
    return 0;
}