
#include "WebAppServer.h"
#include "WebAppServerInternal.h"

#include <stdlib.h>
#include <errno.h>

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
        pc = new serverPort(nr->port, nr->type);
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

serverPort::serverPort(int _port, WebAppType _type)
{
    port = _port;
    type = _type;
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
    cout << "configs on port " << port << ":" << endl;

    serverPort::ConfigRecListIter_t it;
    for (it = configs.begin(); it != configs.end(); it++)
    {
        WebAppServerConfigRecord * cr = *it;
        cout << "   " << *cr << endl;
    }

    startFdThread(makeListeningSocket(port), 1000);
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
    int clientFd = acceptConnection();
    if (clientFd < 0)
    {
        fprintf(stderr, "accept: %s\n", strerror(errno));
        return true;
    }
    //else

    if (type == APP_TYPE_WEBSOCKET)
    {
        WebServerConnectionBase * wscb = 
            new WebSocketConnection(configs, clientFd);
        connections.push_back(wscb);
        wscb->startServer();
    }
    else if (type == APP_TYPE_FASTCGI)
    {
        WebServerConnectionBase * wscb = 
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
