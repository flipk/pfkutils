/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */
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

#ifndef __WEBSOCKET_CLIENT_H__
#define __WEBSOCKET_CLIENT_H__

#include "CircularReader.h"
#include "WebAppMessage.h"
#include "LockWait.h"
#include "BackTrace.h"
#include "fdThreadLauncher.h"

#include <sstream>

namespace WebAppClient {

struct WSClientError : BackTraceUtil::BackTrace
{
    enum WSClientErrVal {
        ERR_URL_MALFORMED,
        ERR_URL_PATH_MALFORMED,
        ERR_PROXY_MALFORMED,
        ERR_PROXY_HOSTNAME,
        ERR_PROXY_IP,
        ERR_URL_HOSTNAME,
        ERR_URL_IP,
        ERR_SOCKET,
        ERR_CONNREFUSED,
        ERR_NOTCONN,
        __NUMERRS
    } err;
    static const std::string errStrings[__NUMERRS];
    WSClientError(WSClientErrVal _e) : err(_e) { }
    /** return a descriptive string matching the error */
    /*virtual*/ const std::string _Format(void) const;
};

class WebSocketClient : fdThreadLauncher,
                        public WaitUtil::Lockable
{
    bool finished;
    static const int MAX_READBUF = 65536 + 0x1000; // 65K
    CircularReader  readbuf;
    bool handle_data(void);
    enum {
        STATE_PROXYRESP, // waitfor proxy CONNECT response
        STATE_HEADERS,   // waiting for ws mime headers
        STATE_CONNECTED // mime headers done, exchanging messages
    } state;
    bool handle_proxyresp(const CircularReaderSubstr &hdr);
    bool handle_wsheader(const CircularReaderSubstr &hdr);
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
    int newfd;
    void generateProxyHeaders(std::ostringstream &hdrs);
    void generateWsHeaders(std::ostringstream &hdrs);
    inline const std::string &hostForConn(void);
    void init_common(const std::string &proxy, const std::string &url);
public:
    WebSocketClient(const std::string &url);
    WebSocketClient(const std::string &proxy, const std::string &url,
                    bool _proxyWsWithConnect);
    virtual ~WebSocketClient(void);
    void startClient(void);
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
