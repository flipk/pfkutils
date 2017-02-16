/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*-  */
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
#include "fdThreadLauncher.h"
#include "LockWait.h"
#include "wsProxyClient-libWebAppServer_proxyTcpServer_proxyMsgs.pb.h"

// ugh. both WebAppConnection and fdThreadLauncher have a doPoll method.
// need to make shim classes so we can implement both separately.

class proxyServerConnWacShim : public WebAppServer::WebAppConnection
{
public:
    virtual ~proxyServerConnWacShim(void) ALLOW_THROWS { }
    virtual bool wacDoPoll(void) = 0;
private:
    /*virtual*/ bool doPoll(void) { return wacDoPoll(); }
};

class proxyServerConnfdtlShim : public fdThreadLauncher
{
public:
    virtual ~proxyServerConnfdtlShim(void) ALLOW_THROWS { }
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
    ~proxyServerConn(void) ALLOW_THROWS;

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
