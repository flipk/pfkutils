/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*-  */

#include "WebAppServer.h"

class proxyServerConn : public WebAppServer::WebAppConnection
{
public:
    proxyServerConn(void);
private:
    ~proxyServerConn(void);
    /*virtual*/ bool onMessage(const WebAppServer::WebAppMessage &m);
    /*virtual*/ bool doPoll(void);
};

class proxyServerConnCB : public WebAppServer::WebAppConnectionCallback
{
public:
    proxyServerConnCB(void) { }
    ~proxyServerConnCB(void) { }
    /*virtual*/ WebAppServer::WebAppConnection * newConnection(void)
    { return new proxyServerConn; }
};
