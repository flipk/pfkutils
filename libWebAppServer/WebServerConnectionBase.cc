/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#include "WebAppServer.h"
#include "WebAppServerInternal.h"

#include <errno.h>

using namespace std;

namespace WebAppServer {

WebServerConnectionBase :: WebServerConnectionBase(
    serverPort::ConfigRecList_t &_configs, int _fd)
    : configs(_configs), readbuf(MAX_READBUF)
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
    if (wac != NULL)
        delete wac;
    wac = NULL;
}

//virtual
bool
WebServerConnectionBase :: doSelect(bool *forRead, bool *forWrite)
{
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

//virtual
void
WebServerConnectionBase :: done(void)
{
    if (wac)
        delete wac;
    wac = NULL;
    deleteMe = true;
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
