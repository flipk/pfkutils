/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#ifndef __WEBAPPSERVERINTERNAL_H__
#define __WEBAPPSERVERINTERNAL_H__

#include <map>

#include "FastCGI.h"
#include "CircularReader.h"
#include "fdThreadLauncher.h"
#include "serverPorts.h"

namespace WebAppServer {

std::ostream &operator<<(std::ostream &ostr, const WebAppType type);

struct WebAppServerConfigRecord {
    WebAppType type;
    int port;
    std::string route;
    WebAppConnectionCallback *cb;
    int pollInterval;
    WebAppServerConfigRecord(WebAppType _type, 
                             int _port,
                             const std::string _route,
                             WebAppConnectionCallback *_cb,
                             int _pollInterval)
        : type(_type), port(_port), route(_route), cb(_cb),
          pollInterval(_pollInterval) { };
    virtual ~WebAppServerConfigRecord(void) { }
};
std::ostream &operator<<(std::ostream &ostr,
                             const WebAppServerConfigRecord &cr);

class Lockable {
    pthread_mutex_t  mutex;
public:
    Lockable(void) {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutex_init(&mutex, &attr);
        pthread_mutexattr_destroy(&attr);
    }
    ~Lockable(void) {
        pthread_mutex_destroy(&mutex);
    }
    void   lock(void) { pthread_mutex_lock  (&mutex); }
    void unlock(void) { pthread_mutex_unlock(&mutex); }
};

class Lock {
    Lockable *obj;
public:
    Lock( Lockable *_obj ) : obj(_obj) { obj->lock(); }
    ~Lock(void) { obj->unlock(); }
};

struct WebAppServerFastCGIConfigRecord : public WebAppServerConfigRecord,
                                         public Lockable {
    WebAppServerFastCGIConfigRecord(WebAppType _type, 
                                    int _port,
                                    const std::string _route,
                                    WebAppConnectionCallback *_cb,
                                    int _pollInterval);
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
                                    public Lockable {
public:
    WebAppConnectionDataFastCGI(void) { waiter = NULL; }
    virtual ~WebAppConnectionDataFastCGI(void) { }
    /*virtual*/ void sendMessage(const WebAppMessage &m);
    std::list<std::string> outq; //base64
    WebFastCGIConnection * waiter;
    static const int maxIdleTime = 30; // in seconds
    time_t lastCall;
    // object should be locked when calling this.
    void sendFrontMessage(void);
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
    static const int MAX_READBUF = 65536;
    friend class serverPort;
protected:
    serverPort::ConfigRecList_t &configs;
    WebAppServerConfigRecord * config;
    WebAppConnection * wac;
    CircularReader readbuf;
    std::string resource;
    bool deleteMe;
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
    virtual void sendMessage(const WebAppMessage &m) = 0;
};

class WebSocketConnection : public WebServerConnectionBase {
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

    // return false to close
    bool handle_header(void);
    bool handle_header_line(const CircularReaderSubstr &headerLine);
    bool handle_message(void);
    void send_handshake_response(void);

public:
    WebSocketConnection(serverPort::ConfigRecList_t &_configs, int _fd);
    /*virtual*/ void sendMessage(const WebAppMessage &m);
};

class WebFastCGIConnection : public WebServerConnectionBase {
    // return false to close
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
