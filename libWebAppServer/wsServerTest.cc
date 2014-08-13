
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
    WebAppServer::WebAppServer         server;


    serverConfig.addWebsocket(1081, "/websocket/test",
                              &appCallback, /*poll*/-1);

    server.start(&serverConfig);

    while (1)
        sleep(1);


    return 0;
}
