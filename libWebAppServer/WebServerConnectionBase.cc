/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#include "WebAppServer.h"
#include "WebAppServerInternal.h"

#include <errno.h>

using namespace std;

namespace WebAppServer {

WebServerConnectionBase :: WebServerConnectionBase(
    serverPort::ConfigRecList_t &_configs, int _fd)
    : tempFd(_fd), configs(_configs), readbuf(MAX_READBUF)
{
    wac = NULL;
    config = NULL;
    deleteMe = false;
    // i wanted to startFdThread here, but i can't
    // because i have pure virtual methods that aren't filled out
    // until my derived constructor completes, and startFdThread
    // will call one or more of them too  quickly.
}

//virtual
WebServerConnectionBase :: ~WebServerConnectionBase(void)
{
    stopFdThread();
    // it is the derived object's responsibility to clean
    // up the wac.  websocket will delete it, fastcgi will not.
    close(fd);
}

//virtual
bool
WebServerConnectionBase :: doSelect(bool *forRead, bool *forWrite)
{
    if (readbuf.remaining() == 0)
        *forRead = false;
    else
        *forRead = true;
    *forWrite = false;
    return true;
}

//virtual
bool
WebServerConnectionBase :: handleWriteSelect(int serverFd)
{
    return false; // not used
}

//virtual
bool
WebServerConnectionBase :: handleReadSelect(int fd)
{
    int cc;

    if (readbuf.remaining() == 0)
    {
        cerr << "read : buffer is full!" << endl;
        return false;
    }

    cc = readbuf.readFd(fd);
    if (cc < 0)
        cerr << "read : " << strerror(errno) << endl;
    if (cc == 0)
    {
        handleSomeData();
        cerr << "read: end of data stream" << endl;
    }
    if (cc <= 0)
        return false;

    return handleSomeData();
}

bool
WebServerConnectionBase :: findResource(void)
{
    serverPort::ConfigRecListIter_t it;
    for (it = configs.begin(); it != configs.end(); it++)
    {
        config = *it;
        if (resource == config->route)
            return true;
        config = NULL;
    }
    return false;
}

} // namespace WebAppServer
