
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
    allowReads = false;
    make_nonblocking();
    sequence = 1;
}

/*virtual*/
proxyClientConn :: ~proxyClientConn(void)
{
    close(fd_interface::fd);
}

/*virtual*/
fd_interface::rw_response
proxyClientConn :: read ( fd_mgr *_mgr )
{
    if (checkFinished())
    {
        printf("proxyClientConn :: read: ws is finished\n");
        return DEL;
    }
    char buf[READ_BUFFER_SIZE];
    int cc = ::read(fd_interface::fd, buf, sizeof(buf));
    pm_out.Clear();
    if (cc <= 0)
    {
        pm_out.set_type(PMT_CLOSING);
        pm_out.mutable_closing()->set_reason("tcp proxy rcvd close");
        sendProxyMsg();
        return DEL;
    }
    pm_out.set_type(PMT_DATA);
    pm_out.mutable_data()->set_data(buf, cc);
    sendProxyMsg();
    return OK;
}

/*virtual*/
void
proxyClientConn :: select_rw ( fd_mgr *_mgr, bool * do_read, bool * do_write )
{
    *do_read = allowReads; // xxx maybe not if remote hasn't conn'd yet
    *do_write = false;
}

void
proxyClientConn :: sendProxyMsg(void)
{
    string buf;
    pm_out.set_sequence(sequence++);
    pm_out.SerializeToString(&buf);
    const WebAppMessage wamPM(WS_TYPE_BINARY, buf);
    WaitUtil::Lock  lock(&sendLock);
    sendMessage(wamPM);
}

/*virtual*/ void
proxyClientConn :: onConnect(void)
{
    ProxyMsg  pm;

    printf("PFK client onConnect\n");

    pm_out.Clear();
    pm_out.set_type(PMT_PROTOVERSION);
    pm_out.mutable_protover()->set_version(PMT_PROTO_VERSION_NUMBER);

    sendProxyMsg();
}

/*virtual*/ void
proxyClientConn :: onDisconnect(void)
{
    printf("PFK client onDisconnect\n");
    allowReads = false;
    do_close = true;
}

/*virtual*/ bool
proxyClientConn :: onMessage(const WebAppServer::WebAppMessage &m)
{
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
        pm_out.Clear();
        pm_out.set_type(PMT_CLOSING);
        pm_out.mutable_closing()->set_reason("pm parse failed");
        sendProxyMsg();
        return false;
    }

    bool ret = true;
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
            ret = false;
        }
        else
            allowReads = true;
        break;

    case PMT_CLOSING:
        cout << "remote closing reason : "
             << pm_in.closing().reason() << endl;
        ret = false;
        break;

    case PMT_DATA:
    {
        int cc = 0;
        if (pm_in.data().data().length() > 0)
        {
            cc = ::write(fd_interface::fd,
                         pm_in.data().data().c_str(),
                         pm_in.data().data().length());
            if (cc != pm_in.data().data().length())
                printf("PFK ERROR write of %d != %d\n",
                       cc, pm_in.data().data().length());
        }
        if (cc <= 0)
            ret = false;
        break;
    }
    }

    return ret;
}
