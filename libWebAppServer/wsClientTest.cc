
#include "WebSocketClient.h"

#include <stdlib.h>

#if 1 // 1 to use proxy
//#define MY_PROXY "wwwgate0.mot.com:1080"
#define MY_PROXY "10.0.0.15:1080"
#endif

#if 0 // 1 to use 'CONNECT' with proxy
#define PROXY_CONNECT true
#else
#define PROXY_CONNECT false
#endif

#define MY_URL "ws://10.0.0.3:1081/websocket/test"

using namespace std;

class myClient : public WebAppClient::WebSocketClient
{
public:
    myClient(const std::string &proxy, const std::string &url,
             bool withConnect)
        : WebSocketClient(proxy,url,withConnect) { }
    myClient(const std::string &url) : WebSocketClient(url) { }
    ~myClient(void)
    {
    }
    /*virtual*/ void onConnect(void)
    {
        cout << "onConnect called!" << endl;
    }
    /*virtual*/ void onDisconnect(void)
    {
        cout << "onDisconnect called!" << endl;
    }
    /*virtual*/ bool onMessage(const WebAppServer::WebAppMessage &m)
    {
        cout << "onMessage called! got : " << m.buf << endl;
        return true;
    }
};


int
main()
{
#ifdef MY_PROXY
    myClient   wsClient(MY_PROXY, MY_URL, PROXY_CONNECT);
#else
    myClient   wsClient(MY_URL);
#endif

    while (1)
    {
        sleep(1);
        if (wsClient.checkFinished())
            break;
        ostringstream  ostr;
        ostr << random();
        cout << "sending " << ostr.str() << endl;
        wsClient.sendMessage(
            WebAppServer::WebAppMessage(
                WebAppServer::WS_TYPE_BINARY, ostr.str()));
    }

    return 0;
}
