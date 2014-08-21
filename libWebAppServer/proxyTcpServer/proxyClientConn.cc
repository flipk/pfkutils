
#include "proxyClientConn.h"

using namespace std;
using namespace proxyTcp;
using namespace WebAppServer;
using namespace WebAppClient;

proxyClientConn :: proxyClientConn(const string &proxy,
                                   const string &url,
                                   bool withConnect, int new_fd)
    : WebSocketClient(proxy, url, withConnect)
{
    fd_interface::fd = new_fd;
    make_nonblocking();
}

/*virtual*/
proxyClientConn :: ~proxyClientConn(void)
{
    pm_out.Clear();
    pm_out.set_type(PMT_CLOSING);
    pm_out.mutable_closing()->set_reason("destroying connection");
    sendProxyMsg();
    close(fd_interface::fd);
}

/*virtual*/
fd_interface::rw_response
proxyClientConn :: read ( fd_mgr * )
{
    if (checkFinished())
    {
        printf("proxyClientConn :: read: ws is finished\n");
        return DEL;
    }
    char buf[64];
    int cc = ::read(fd_interface::fd, buf, 64);
    printf("read returns %d\n", cc);
    if (cc <= 0)
        return DEL;
    // xxx
    return OK;
}

/*virtual*/
void
proxyClientConn :: select_rw ( fd_mgr *, bool * do_read, bool * do_write )
{
    *do_read = true; // xxx maybe not if remote hasn't conn'd yet
    *do_write = false;
}

void
proxyClientConn :: sendProxyMsg(void)
{
    string buf;
    pm_out.SerializeToString(&buf);
    const WebAppMessage wamPM(WS_TYPE_BINARY, buf);
    WaitUtil::Lock  lock(&sendLock);
    sendMessage(wamPM);
}

/*virtual*/ void
proxyClientConn :: onConnect(void)
{
    ProxyMsg  pm;

    printf("PFK client sending msg at onConnect\n");

    pm_out.Clear();
    pm_out.set_type(PMT_PROTOVERSION);
    pm_out.mutable_protover()->set_version(PMT_PROTO_VERSION_NUMBER);

    sendProxyMsg();
}

/*virtual*/ void
proxyClientConn :: onDisconnect(void)
{
    printf("PFK client onDisconnect\n");
}

/*virtual*/ bool
proxyClientConn :: onMessage(const WebAppServer::WebAppMessage &m)
{
    printf("PFK client onMessage\n");

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
        if (pm_in.protover().version() != PMT_PROTO_VERSION_NUMBER)
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
