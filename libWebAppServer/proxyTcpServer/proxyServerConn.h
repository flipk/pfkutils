/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*-  */

#include "WebAppServer.h"
#include "fdThreadLauncher.h"
#include "LockWait.h"
#include "proxyMsgs.pb.h"

// ugh. both WebAppConnection and fdThreadLauncher have a doPoll method.
// need to make shim classes so we can implement both separately.

class proxyServerConnWacShim : public WebAppServer::WebAppConnection
{
public:
    virtual bool wacDoPoll(void) = 0;
private:
    /*virtual*/ bool doPoll(void) { return wacDoPoll(); }
};

class proxyServerConnfdtlShim : public WebAppServer::fdThreadLauncher
{
public:
    virtual bool fdtlDoPoll(void) = 0;
private:
    /*virtual*/ bool doPoll(void) { return fdtlDoPoll(); }
};

class proxyServerConn : public proxyServerConnWacShim,
                        public proxyServerConnfdtlShim
{
public:
    proxyServerConn(const std::string &_destHost,
                    int _destPort);
private:
    static const int READ_BUFFER_SIZE = 4096;

    std::string  destHost;
    int          destPort;
    int          sequence;

    proxyTcp::ProxyMsg  pm_in;
    proxyTcp::ProxyMsg  pm_out;
    WaitUtil::Lockable  sendLock;
    void sendProxyMsg(void);
    ~proxyServerConn(void);

    // implement WebAppConnection

    /*virtual*/ void onConnect(void);
    /*virtual*/ void onDisconnect(void);
    /*virtual*/ bool onMessage(const WebAppServer::WebAppMessage &m);
    /*virtual*/ bool wacDoPoll(void);

    // implement fdThreadLauncher

    /*virtual*/ bool doSelect(bool *forRead, bool *forWrite);
    /*virtual*/ bool handleReadSelect(int fd);
    /*virtual*/ bool handleWriteSelect(int fd);
    /*virtual*/ bool fdtlDoPoll(void);
    /*virtual*/ void done(void);

    // private

    bool startConnection(void);
};

class proxyServerConnCB : public WebAppServer::WebAppConnectionCallback
{
    std::string  destHost;
    int          destPort;
public:
    proxyServerConnCB(const std::string &_destHost, int _destPort)
        : destHost(_destHost), destPort(_destPort) { }
    ~proxyServerConnCB(void) { }
    /*virtual*/ WebAppServer::WebAppConnection * newConnection(void)
    { return new proxyServerConn(destHost, destPort); }
};
