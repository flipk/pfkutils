/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*-  */

#include "WebSocketClient.h"
#include "fd_mgr.H"
#include "proxyMsgs.pb.h"

class proxyClientConn : public fd_interface,
                        public WebAppClient::WebSocketClient
{
public:
    proxyClientConn(const std::string &proxy, const std::string &url,
                    bool withConnect, int new_fd);
private:
    /*virtual*/ ~proxyClientConn(void);
    proxyTcp::ProxyMsg  pm_in;
    proxyTcp::ProxyMsg  pm_out;
    WaitUtil::Lockable  sendLock;
    void sendProxyMsg(void);

    // fd_interface

    /*virtual*/ rw_response read ( fd_mgr * );
    /*virtual*/ void select_rw ( fd_mgr *, bool * do_read, bool * do_write );

    // WebSocketClient

    /*virtual*/ void onConnect(void);
    /*virtual*/ void onDisconnect(void);
    /*virtual*/ bool onMessage(const WebAppServer::WebAppMessage &m);
};
