
#include "WebSocketClient.h"

class proxyClientConn : public WebAppClient::WebSocketClient
{
public:
    proxyClientConn(const std::string &proxy, const std::string &url,
             bool withConnect);
    proxyClientConn(const std::string &url);
    ~proxyClientConn(void);
    /*virtual*/ void onConnect(void);
    /*virtual*/ void onDisconnect(void);
    /*virtual*/ bool onMessage(const WebAppServer::WebAppMessage &m);
};
