
#include "proxyServerConn.h"
#include "proxyMsgs.pb.h"

using namespace std;
using namespace WebAppServer;

proxyServerConn :: proxyServerConn(void)
{
}


proxyServerConn :: ~proxyServerConn(void)
{
}

/*virtual*/
bool
proxyServerConn :: onMessage(const WebAppServer::WebAppMessage &m)
{
    return false;
}

/*virtual*/
bool
proxyServerConn :: doPoll(void)
{
    return false;
}
