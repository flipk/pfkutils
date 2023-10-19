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

#ifndef __WEBAPPSERVERINTERNAL_H__
#define __WEBAPPSERVERINTERNAL_H__

#include <map>
#include <netinet/in.h>

#include "FastCGI.h"
#include "CircularReader.h"
#include "fdThreadLauncher.h"
#include "serverPorts.h"
#include "LockWait.h"

namespace WebAppServer {

std::ostream &operator<<(std::ostream &ostr, const WebAppType type);

struct WebAppServerConfigRecord {
    WebAppType type;
    int port;
    std::string route;
    WebAppConnectionCallback *cb;
    int pollInterval;
    int msgTimeout;
    std::string ipaddr;
    WebAppServerConfigRecord(WebAppType _type, 
                             int _port,
                             const std::string _route,
                             WebAppConnectionCallback *_cb,
                             int _pollInterval,
                             int _msgTimeout)
        : type(_type), port(_port), route(_route), cb(_cb),
          pollInterval(_pollInterval),
          msgTimeout(_msgTimeout), ipaddr("") { };
    WebAppServerConfigRecord(WebAppType _type,
                             int _port,
                             const std::string _route,
                             WebAppConnectionCallback *_cb,
                             const std::string _ipaddr,
                             int _pollInterval,
                             int _msgTimeout)
        : type(_type), port(_port), route(_route), cb(_cb),
          pollInterval(_pollInterval),
          msgTimeout(_msgTimeout), ipaddr(_ipaddr) { };
    virtual ~WebAppServerConfigRecord(void) { }
};
std::ostream &operator<<(std::ostream &ostr,
                             const WebAppServerConfigRecord &cr);

struct WebAppServerFastCGIConfigRecord : public WebAppServerConfigRecord,
                                         public WaitUtil::Lockable {
    WebAppServerFastCGIConfigRecord(WebAppType _type, 
                                    int _port,
                                    const std::string _route,
                                    WebAppConnectionCallback *_cb,
                                    int _pollInterval,
                                    int _msgTimeout);
    /*virtual*/ ~WebAppServerFastCGIConfigRecord(void);
    // ConnList key : visitorId cookie
    typedef std::map<std::string,WebAppConnection*> ConnList_t;
    typedef std::map<std::string,WebAppConnection*>::iterator ConnListIter_t;
    ConnList_t conns;
private:
    static void * _thread_entry(void *_this);
    void thread_entry(void);
    int closePipe[2];
    pthread_t thread_id;
};

class WebAppConnectionDataWebsocket;
class WebAppConnectionDataFastCGI;

class WebAppConnectionData {
public:
    virtual ~WebAppConnectionData(void) { }
    virtual void sendMessage(const WebAppMessage &) = 0;
    WebAppConnectionDataWebsocket * ws(void);
    WebAppConnectionDataFastCGI * fcgi(void);
};

class WebSocketConnection;
class WebAppConnectionDataWebsocket : public WebAppConnectionData {
public:
    WebAppConnectionDataWebsocket(WebSocketConnection * _connBase)
        : connBase(_connBase) { }
    virtual ~WebAppConnectionDataWebsocket(void) { }
    /*virtual*/ void sendMessage(const WebAppMessage &m);
    WebSocketConnection * connBase;
};

class WebFastCGIConnection;
class WebAppConnectionDataFastCGI : public WebAppConnectionData,
                                    public WaitUtil::Lockable {
public:
    WebAppConnectionDataFastCGI(void) { waiter = NULL; }
    virtual ~WebAppConnectionDataFastCGI(void) { }
    /*virtual*/ void sendMessage(const WebAppMessage &m);
    std::list<std::string> outq; //base64
    WebFastCGIConnection * waiter;
    static const int maxIdleTime = 4; // in seconds
    time_t lastCall;
    // object should be locked when calling this.
    void sendFrontMessage(WebFastCGIConnection * _waiter);
};

inline WebAppConnectionDataWebsocket * WebAppConnectionData::ws(void)
{ 
    return dynamic_cast<WebAppConnectionDataWebsocket*>(this);
}
inline WebAppConnectionDataFastCGI * WebAppConnectionData::fcgi(void)
{
    return dynamic_cast<WebAppConnectionDataFastCGI*>(this);
}

class WebServerConnectionBase : public fdThreadLauncher {
public:
    static const int MAX_READBUF = 65536 + 0x1000; // 65K
    friend class serverPort;
protected:
    int tempFd;
    serverPort::ConfigRecList_t &configs;
    WebAppServerConfigRecord * config;
    WebAppConnection * wac;
    CircularReader readbuf;
    std::string resource;
    bool deleteMe;
    struct sockaddr_in remote_addr;
    bool findResource(void);
    /*virtual*/ bool doSelect(bool *forRead, bool *forWrite);
    /*virtual*/ bool handleWriteSelect(int serverFd);
    /*virtual*/ bool handleReadSelect(int serverFd);
    // doPoll is left to the derived obj
    // done is left to the derived
    virtual bool handleSomeData(void) = 0; // derived must implement
public:
    WebServerConnectionBase(serverPort::ConfigRecList_t &_configs, int _fd);
    virtual ~WebServerConnectionBase(void);
    virtual void startServer(void) = 0;
    virtual void sendMessage(const WebAppMessage &m) = 0;
    const struct sockaddr_in *get_remote_addr(void) {
        return &remote_addr;
    }
};

class WebSocketConnection : public WebServerConnectionBase {
    /*virtual*/ void startServer(void);
    // return false to close
    /*virtual*/ bool handleSomeData(void);
    /*virtual*/ bool doPoll(void);
    /*virtual*/ void done(void);
    ~WebSocketConnection(void);

    enum {
        STATE_HEADER,   // waiting for mime headers
        STATE_CONNECTED // mime headers done, exchanging messages
    } state;

    static const int GOT_NONE            = 0;
    static const int GOT_HOST            = 1;
    static const int GOT_KEY             = 2;
    static const int GOT_ORIGIN          = 4;
    static const int GOT_VERSION         = 8;
    static const int GOT_RESOURCE        = 16;
    static const int GOT_UPGRADE_FLAG    = 32;
    static const int GOT_CONNECTION_FLAG = 64;
    static const int GOT_ALL             = 127;  // all combined
    int got_flags;

    std::string host;
    std::string origin;
    std::string version;
    std::string key;
    std::string reassembly_buffer;
    WebAppMessageType reassembly_msg_type;

    // return false to close
    bool handle_header(void);
    bool handle_header_line(const CircularReaderSubstr &headerLine);
    bool handle_message(void);
    void send_handshake_response(void);

public:
    WebSocketConnection(serverPort::ConfigRecList_t &_configs, int _fd,
                        const struct sockaddr_in *sa);
    /*virtual*/ void sendMessage(const WebAppMessage &m);
};

class WebFastCGIConnection : public WebServerConnectionBase {
    // return false to close
    /*virtual*/ void startServer(void);
    /*virtual*/ bool handleSomeData(void);
    /*virtual*/ bool doPoll(void);
    /*virtual*/ void done(void);
    ~WebFastCGIConnection(void);
    
    static const int visitorCookieLen = 40;

    static const int maxIdleTime = 30; // in seconds

    enum {
        STATE_BEGIN, // waiting for begin
        STATE_PARAMS, // waiting for params
        STATE_INPUT, // waiting for input
        STATE_BLOCKED, // no output to send, waiting.
        STATE_OUTPUT // sending output
    } state;

    uint16_t requestId;
    FastCGIParamsList * cgiParams;
    FastCGIParamsList * queryStringParams;
    WebAppServerFastCGIConfigRecord * cgiConfig;

    time_t lastCall;

    std::string cookieString;
    std::string stdinBuffer;

    bool registeredWaiter;

    bool handleRecord(const FastCGIRecord *rec);
    bool handleBegin(const FastCGIRecord *rec);
    bool handleStdin(const FastCGIRecord *rec);
    bool handleParams(const FastCGIRecord *rec);
    void printRecord(const FastCGIRecord *rec);
    bool sendRecord(const FastCGIRecord &rec);
    void generateNewVisitorId(std::string &visitorId);
    bool startWac(void);
    bool decodeInput(void);
    bool startOutput(void);

public:
    WebFastCGIConnection(serverPort::ConfigRecList_t &_configs, int _fd);
    /*virtual*/ void sendMessage(const WebAppMessage &m);
};

} // namespace WebAppServer

#endif /* __WEBAPPSERVERINTERNAL_H__ */
