
#include "WebSocketClient.h"

class proxyServerConn : public WebAppClient::WebSocketClient
{
public:
    proxyServerConn(const std::string &proxy, const std::string &url,
             bool withConnect);
    proxyServerConn(const std::string &url);
    ~proxyServerConn(void);
    /*virtual*/ void onConnect(void);
    /*virtual*/ void onDisconnect(void);
    /*virtual*/ bool onMessage(const WebAppServer::WebAppMessage &m);
};
