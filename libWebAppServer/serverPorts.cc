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

#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>

using namespace std;

namespace WebAppServer {

serverPorts::serverPorts(void)
{
}

serverPorts::~serverPorts(void)
{
    serverPorts::portIter_t pit;
    for (pit = portMap.begin(); pit != portMap.end(); pit++ )
    {
        delete pit->second;
    }
}

void
serverPorts::addConfigRec(WebAppServerConfigRecord *nr)
{
    serverPorts::portIter_t pit;
    serverPort * pc;

    pit = portMap.find(nr->port);
    if (pit == portMap.end())
    {
        pc = new serverPort(nr->port, nr->ipaddr, nr->type);
        portMap[nr->port] = pc;
    }
    else
    {
        pc = pit->second;
    }
    if (pc->type != nr->type)
    {
        fprintf(stderr, "ERROR: cannot have different server types "
                "on the same port number\n");
        exit(1);
    }
    pc->addConfigRec(nr);
}

serverPort::serverPort(int _port, std::string _ipaddr, WebAppType _type)
{
    port = _port;
    type = _type;
    ipaddr = _ipaddr;
}

serverPort::~serverPort(void)
{
    serverPort::ConfigRecListIter_t it;
    for (it = configs.begin(); it != configs.end(); )
    {
        // do not delete WebAppServerConfigRecord*'s, 
        // because they're shared with WebAppServer's Config obj.
        it = configs.erase(it);
    }
}

void
serverPort::addConfigRec(WebAppServerConfigRecord *nr)
{
    configs.push_back(nr);
}

void serverPort::startThread(void)
{
    uint32_t ip;
    cout << "configs on port " << port << ":" << endl;

    serverPort::ConfigRecListIter_t it;
    for (it = configs.begin(); it != configs.end(); it++)
    {
        WebAppServerConfigRecord * cr = *it;
        cout << "   " << *cr << endl;
    }

    if(0 >= inet_pton(AF_INET, ipaddr.c_str(), &ip))
    {
       // fail, just bind to everything
       ip = INADDR_ANY;
    }
    else
    {
       // inet_pton does the endian swap for you, and makeListeningSocket
       // wants it in host form, so let's just put it back for now.
       ip = ntohl(ip);
    }

    startFdThread(makeListeningSocket(ip, port), 1000);
}

void
serverPort::stopThread(void)
{
    stopFdThread();
    close(fd);
    ConnectionList::iterator it;
    for (it = connections.begin(); it != connections.end(); )
    {
        WebServerConnectionBase * wscb = *it;
        delete wscb;
        it = connections.erase(it);
    }
}

bool
serverPort :: doSelect(bool *forRead, bool *forWrite)
{
    *forRead = true;
    *forWrite = false;
    return true;
}

bool
serverPort :: handleReadSelect(int fd)
{
    struct sockaddr_in sa;
    int clientFd = acceptConnection(&sa);
    if (clientFd < 0)
    {
        fprintf(stderr, "accept: %s\n", strerror(errno));
        return true;
    }
    //else

    // Set socket options on the new connection
    serverPort::ConfigRecListIter_t it;
    for (it = configs.begin(); it != configs.end(); it++)
    {
        if (port == (*it)->port)
        {
            setSocketOpts(clientFd, (*it)->msgTimeout);
        }
    }

    if (type == APP_TYPE_WEBSOCKET)
    {
        WebServerConnectionBase * wscb = 
            new WebSocketConnection(configs, clientFd, &sa);
        connections.push_back(wscb);
        wscb->startServer();
    }
    else if (type == APP_TYPE_FASTCGI)
    {
        WebServerConnectionBase * wscb = 
// todo, someday
//          new WebFastCGIConnection(configs, clientFd, &sa);
            new WebFastCGIConnection(configs, clientFd);
        connections.push_back(wscb);
        wscb->startServer();
    }
    return true;
}

bool
serverPort :: handleWriteSelect(int fd)
{
    return false; // should never happen
}

bool
serverPort :: doPoll(void)
{
    connectionsIterator_t  it;
    for (it = connections.begin(); it != connections.end(); )
    {
        WebServerConnectionBase * wscb = *it;
        if (wscb->deleteMe)
        {
            delete wscb;
            it = connections.erase(it);
        }
        else
        {
            it++;
        }
    }
    return true;
}

void
serverPort :: done(void)
{
    delete this;
}

} // namespace WebAppServer
