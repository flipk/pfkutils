
#include "proxyServerConn.h"
#include "proxyMsgs.pb.h"

#include <stdio.h>
#include <iostream>

using namespace std;
using namespace proxyTcp;
using namespace WebAppServer;

proxyServerConn :: proxyServerConn(void)
{
}

proxyServerConn :: ~proxyServerConn(void)
{
}

void
proxyServerConn :: sendProxyMsg(void)
{
    string buf;
    pm_out.SerializeToString(&buf);
    const WebAppMessage wamPM(WS_TYPE_BINARY, buf);
    WaitUtil::Lock  lock(&sendLock);
    sendMessage(wamPM);
}

/*virtual*/
void
proxyServerConn :: onConnect(void)
{
    printf("PFK server sending msg\n");

    pm_out.Clear();
    pm_out.set_type(PMT_PROTOVERSION);
    pm_out.mutable_protover()->set_version(PMT_PROTO_VERSION_NUMBER);
    sendProxyMsg();
}

/*virtual*/
void
proxyServerConn :: onDisconnect(void)
{
}

/*virtual*/
bool
proxyServerConn :: onMessage(const WebAppServer::WebAppMessage &m)
{
    printf("PFK server got message\n");

    if (m.type == WS_TYPE_CLOSE)
    {
        cout << "closing due to WS CLOSE" << endl;
        return false;
    }
    if (m.type != WS_TYPE_BINARY)
    {
        cout << "only binary supported" << endl;
        return false;
    }
    pm_in.Clear();
    if (pm_in.ParseFromString(m.buf) == false)
    {
        cout << "pm parse failed" << endl;
        return false;
    }

    switch (pm_in.type())
    {
    case PMT_PROTOVERSION:
        cout << "got proto version "
             << pm_in.protover().version() << endl;
        if (pm_in.protover().version() != 3) //PMT_PROTO_VERSION_NUMBER)
        {
            cerr << "protocol version mismatch!" << endl;
            pm_out.Clear();
            pm_out.set_type(PMT_CLOSING);
            pm_out.mutable_closing()->set_reason("protocol version mismatch");
            sendProxyMsg();
            return false;
        }
        break;
    case PMT_CLOSING:
        cout << "remote closing reason : "
             << pm_in.closing().reason() << endl;
        return false;
    }

    return true;
}

/*virtual*/
bool
proxyServerConn :: doPoll(void)
{
    return false;
}
