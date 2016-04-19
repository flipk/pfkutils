/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#ifndef __SERVER_PORTS_H__
#define __SERVER_PORTS_H__

#include <map>

namespace WebAppServer {

enum WebAppType { APP_TYPE_WEBSOCKET, APP_TYPE_FASTCGI };

class serverPort : public fdThreadLauncher {
    std::list<WebServerConnectionBase*> connections;
    typedef std::list<WebServerConnectionBase*>::iterator connectionsIterator_t;
    /*virtual*/ bool doSelect(bool *forRead, bool *forWrite);
    /*virtual*/ bool handleReadSelect(int serverFd);
    /*virtual*/ bool handleWriteSelect(int serverFd);
    /*virtual*/ bool doPoll(void);
    /*virtual*/ void done(void);
public:
    WebAppType type;
    int port;
    typedef std::list<WebAppServerConfigRecord*> ConfigRecList_t;
    typedef std::list<WebAppServerConfigRecord*>::iterator ConfigRecListIter_t;
    typedef std::list<WebAppServerConfigRecord*>::const_iterator ConfigRecListIterC_t;
    ConfigRecList_t configs;
    serverPort(int _port, WebAppType _type);
    ~serverPort(void);
    void addConfigRec(WebAppServerConfigRecord *nr);
    void startThread(void);
    void stopThread(void);
};

struct serverPorts {
    std::map<int,serverPort*> portMap;
    typedef std::map<int,serverPort*>::iterator portIter_t;
    serverPorts(void);
    ~serverPorts(void);
    void addConfigRec(WebAppServerConfigRecord *nr);
};

} // namespace WebAppServer

#endif /* __SERVER_PORTS_H__ */
