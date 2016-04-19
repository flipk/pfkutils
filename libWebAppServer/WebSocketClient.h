/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#ifndef __WEBSOCKET_CLIENT_H__
#define __WEBSOCKET_CLIENT_H__

#include "CircularReader.h"
#include "WebAppMessage.h"
#include "LockWait.h"
#include "fdThreadLauncher.h"

#include <sstream>

namespace WebAppClient {

struct WSClientError
{
    enum err {
        ERR_URL_MALFORMED,
        ERR_URL_PATH_MALFORMED,
        ERR_PROXY_MALFORMED,
        ERR_PROXY_HOSTNAME,
        ERR_PROXY_IP,
        ERR_URL_HOSTNAME,
        ERR_URL_IP,
        ERR_SOCKET,
        ERR_CONNREFUSED,
        ERR_NOTCONN
    };
    err e;
    WSClientError(err _e) : e(_e) { }
};

class WebSocketClient : fdThreadLauncher,
                        public WaitUtil::Lockable
{
    bool finished;
    static const int MAX_READBUF = 65536;
    WebAppServer::CircularReader  readbuf;
    bool handle_data(void);
    enum {
        STATE_PROXYRESP, // waitfor proxy CONNECT response
        STATE_HEADERS,   // waiting for ws mime headers
        STATE_CONNECTED // mime headers done, exchanging messages
    } state;
    bool handle_proxyresp(const WebAppServer::CircularReaderSubstr &hdr);
    bool handle_wsheader(const WebAppServer::CircularReaderSubstr &hdr);
    bool handle_message(void);
    // fdThreadLauncher methods below
    /*virtual*/ bool doSelect(bool *forRead, bool *forWrite);
    /*virtual*/ bool handleReadSelect(int fd);
    /*virtual*/ bool handleWriteSelect(int fd);
    /*virtual*/ bool doPoll(void);
public:
    /*virtual*/ void done(void);
private:
    static const int  GOT_NONE               = 0;
    static const int  GOT_SWITCHING          = 1;
    static const int  GOT_CONNECTION_UPGRADE = 2;
    static const int  GOT_UPGRADE_WEBSOCKET  = 4;
    static const int  GOT_ACCEPT             = 8;
    static const int  GOT_ALL                = 15; // all combined
    int got_flags;
    bool bUserConnCallback;
    bool bProxy;
    bool bProxyWsWithConnect;
    std::string proxyHost, proxyIp, urlHost, urlIp, urlPath;
    std::string secWebsocketKey;
    std::string secWebsocketKeyResponse;
    int proxyPort, urlPort;
    void generateProxyHeaders(std::ostringstream &hdrs);
    void generateWsHeaders(std::ostringstream &hdrs);
    inline const std::string &hostForConn(void);
    void init_common(const std::string &proxy, const std::string &url);
public:
    WebSocketClient(const std::string &url);
    WebSocketClient(const std::string &proxy, const std::string &url,
                    bool _proxyWsWithConnect);
    virtual ~WebSocketClient(void);
    bool sendMessage(const WebAppServer::WebAppMessage &m);
    bool checkFinished(void) { return finished; }
    // derived class must implement
    virtual void onConnect(void) = 0;
    virtual void onDisconnect(void) = 0;
    virtual bool onMessage(const WebAppServer::WebAppMessage &m) = 0;
};

// impl

inline const std::string &WebSocketClient::hostForConn(void)
{
    if (urlHost.length() > 0)
        return urlHost;
    //else
    return urlIp;
}

} // namespace WebAppClient

#endif /* __WEBSOCKET_CLIENT_H__ */
