
#include "proxyServerConn.h"
#include "proxyMsgs.pb.h"

using namespace std;
using namespace proxyTcp;
using namespace WebAppServer;

proxyServerConn :: proxyServerConn(void)
{
    ProxyMsg  pm;

    pm.set_type(PMT_PROTOVERSION);
    pm.mutable_protover()->set_version(PMT_PROTO_VERSION_NUMBER);
    std::string buf;
    pm.SerializeToString(&buf);
    const WebAppMessage wamPM(WS_TYPE_BINARY, buf);

    sendMessage(wamPM);
}

proxyServerConn :: ~proxyServerConn(void)
{
}

/*virtual*/
bool
proxyServerConn :: onMessage(const WebAppServer::WebAppMessage &m)
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

/*virtual*/
bool
proxyServerConn :: doPoll(void)
{
    return false;
}
