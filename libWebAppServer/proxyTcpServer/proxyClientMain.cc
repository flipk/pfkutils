
/*
PROXY_CONNECT=1
PROXY=wwwgate0.mot.com:1080
PROXY=10.0.0.15:1080
URL=ws://10.0.0.3:1081/websocket/proxy
URL=ws://leviathan.phillipknaack.com/websocket/levproto
LISTEN_PORT=2222
*/

#include "proxyClientConn.h"
#include "proxyClientTcpAcceptor.h"

#define DEFAULT_LISTEN_PORT 2222
#define DEFAULT_URL "ws://leviathan.phillipknaack.com/websocket/levproto"

using namespace std;
using namespace WebAppServer;
using namespace WebAppClient;

int
main()
{
    fd_mgr   mgr(false);

    int listenPort = DEFAULT_LISTEN_PORT;
    char * listenPortStr = getenv("LISTEN_PORT");
    if (listenPortStr != NULL)
        listenPort = atoi(listenPortStr);

    bool proxyConnect = false;
    if (getenv("PROXY_CONNECT"))
        proxyConnect = true;
    std::string proxy = "";
    char * proxyStr = getenv("PROXY");
    if (proxyStr != NULL)
        proxy = proxyStr;
    std::string url = DEFAULT_URL;
    char * urlStr = getenv("URL");
    if (urlStr != NULL)
        url = urlStr;

    mgr.register_fd(new proxyClientTcpAcceptor(listenPort, proxy,
                                               proxyConnect, url));
    while (1)
    {
        struct timeval tv = { 0, 100000 };
        mgr.loop(&tv);
    }
    return 0;
}
