
#include "proxyClientConn.h"
#include "proxyMsgs.pb.h"

using namespace std;
using namespace proxyTcp;
using namespace WebAppServer;
using namespace WebAppClient;

proxyClientConn :: proxyClientConn(const string &proxy,
                                   const string &url,
                                   bool withConnect)
    : WebSocketClient(proxy,url,withConnect)
{
}

proxyClientConn :: proxyClientConn(const string &url)
    : WebSocketClient(url)
{
}

proxyClientConn :: ~proxyClientConn(void)
{
}

/*virtual*/ void
proxyClientConn :: onConnect(void)
{
    ProxyMsg  pm;

    pm.set_type(PMT_PROTOVERSION);
    pm.mutable_protover()->set_version(PMT_PROTO_VERSION_NUMBER);
    string buf;
    pm.SerializeToString(&buf);
    const WebAppMessage wamPM(WS_TYPE_BINARY, buf);

    sendMessage(wamPM);
}

/*virtual*/ void
proxyClientConn :: onDisconnect(void)
{
}

/*virtual*/ bool
proxyClientConn :: onMessage(const WebAppServer::WebAppMessage &m)
{
    ProxyMsg  pm;

    if (m.type == WS_TYPE_CLOSE)
    {
        // ?
    }

    if (m.type != WS_TYPE_BINARY)
    {
        // ?
    }

    if (pm.ParseFromString(m.buf) == false)
    {
        // ?
    }

    switch (pm.type())
    {
    case PMT_PROTOVERSION:
        break;
    }

    return false;
}
