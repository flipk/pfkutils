
#include "WebAppServer.h"
#include "proxyServerConn.h"
#include <stdlib.h>

using namespace std;
using namespace WebAppServer;

int
main()
{
    WebAppServerConfig  serverConfig;
    proxyServerConnCB   proxyCB;
    WebAppServer::WebAppServer        server;

    char * portNumEnv = getenv("PROXY_WS_PORT");
    int portNum = 1081;
    if (portNumEnv)
        portNum = atoi(portNumEnv);

    const char * proxyPathEnv = getenv("PROXY_WS_PATH");
    if (proxyPathEnv == NULL)
        proxyPathEnv = "/websocket/proxy";

    serverConfig.addWebsocket(portNum, proxyPathEnv,
                              &proxyCB, /*poll*/ -1);

    server.start(&serverConfig);

    while (1)
        sleep(1);

    return 0;
}
