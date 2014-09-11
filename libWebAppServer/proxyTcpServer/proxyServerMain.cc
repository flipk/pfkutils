
#include "WebAppServer.h"
#include "proxyServerConn.h"
#include <stdlib.h>

using namespace std;
using namespace WebAppServer;

int
main()
{
    char * destHostStr = getenv("DEST_HOST");
    if (destHostStr == NULL)
    {
        cerr << "please set DEST_HOST" << endl;
        return 1;
    }

    char * destPortStr = getenv("DEST_PORT");
    if (destPortStr == NULL)
    {
        cerr << "please set DEST_PORT" << endl;
        return 1;
    }

    WebAppServerConfig  serverConfig;
    proxyServerConnCB   proxyCB(destHostStr, atoi(destPortStr));
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
