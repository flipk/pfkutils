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
