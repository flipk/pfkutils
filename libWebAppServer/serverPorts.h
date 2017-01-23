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

#ifndef __SERVER_PORTS_H__
#define __SERVER_PORTS_H__

#include <map>

namespace WebAppServer {

enum WebAppType { APP_TYPE_WEBSOCKET, APP_TYPE_FASTCGI };

class serverPort : public fdThreadLauncher {
    typedef std::list<WebServerConnectionBase*> ConnectionList;
    ConnectionList connections;
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
