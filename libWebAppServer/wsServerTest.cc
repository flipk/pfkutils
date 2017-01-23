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

#include "WebAppServer.h"

#include <unistd.h>
#include <iostream>
#include <sstream>
#include <stdlib.h>

using namespace std;

class myAppConn : public WebAppServer::WebAppConnection
{
public:
    myAppConn(void)
    {
    }
private:
    ~myAppConn(void)
    {
    }
    /*virtual*/ bool onMessage(const WebAppServer::WebAppMessage &m)
    {
        cout << "got onMessage ! got : " << m.buf << endl;
        sendMessage(
            WebAppServer::WebAppMessage(
                WebAppServer::WS_TYPE_BINARY, m.buf));
        return true;
    }
    /*virtual*/ bool doPoll(void)
    {
        return false;
    }
};

class myAppCallback : public WebAppServer::WebAppConnectionCallback
{
public:
    myAppCallback(void)
    {
    }
    ~myAppCallback(void)
    {
    }
    /*virtual*/ WebAppServer::WebAppConnection * newConnection(void)
    {
        return new myAppConn;
    }
};


int
main()
{
    WebAppServer::WebAppServerConfig   serverConfig;
    myAppCallback                      appCallback;
    WebAppServer::WebAppServerServer   server;


    serverConfig.addWebsocket(1081, "/websocket/test",
                              &appCallback, /*poll*/-1);

    server.start(&serverConfig);

    while (1)
        sleep(1);


    return 0;
}
