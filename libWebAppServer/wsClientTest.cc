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

bool connected = false;

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
        connected = true;
    }
    /*virtual*/ void onDisconnect(void)
    {
        cout << "onDisconnect called!" << endl;
        connected = false;
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

    wsClient.startClient();

    while (1)
    {
        sleep(1);
        if (wsClient.checkFinished())
            break;
        if (connected)
        {
            ostringstream  ostr;
            ostr << random();
            cout << "sending " << ostr.str() << endl;
            wsClient.sendMessage(
                WebAppServer::WebAppMessage(
                    WebAppServer::WS_TYPE_BINARY, ostr.str()));
        }
        else
        {
            cout << "not yet connected, waiting\n";
        }
    }

    return 0;
}
