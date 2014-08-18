
#include "proxyClientConn.h"

proxyServerConn :: proxyServerConn(const std::string &proxy,
                                   const std::string &url,
                                   bool withConnect)
    : WebSocketClient(proxy,url,withConnect)
{
}

proxyServerConn :: proxyServerConn(const std::string &url)
    : WebSocketClient(url)
{
}

proxyServerConn :: ~proxyServerConn(void)
{
}

/*virtual*/ void
proxyServerConn :: onConnect(void)
{
}

/*virtual*/ void
proxyServerConn :: onDisconnect(void)
{
}

/*virtual*/ bool
proxyServerConn :: onMessage(const WebAppServer::WebAppMessage &m)
{
    return false;
}
