
#include "proxyServerConn.h"
#include "proxyMsgs.pb.h"

#include <stdio.h>
#include <iostream>

using namespace std;
using namespace proxyTcp;
using namespace WebAppServer;

proxyServerConn :: proxyServerConn(const std::string &_destHost, int _destPort)
    : destHost(_destHost), destPort(_destPort)
{
    sequence = 1;
}

proxyServerConn :: ~proxyServerConn(void)
{
    stopFdThread();
}

bool
proxyServerConn :: startConnection(void)
{
    int new_fd = fdThreadLauncher::makeConnectingSocket(destHost,
                                                        destPort);

    if (new_fd < 0)
        return false;

    startFdThread(new_fd, /*pollInterval*/ 100);

    return true;
}

void
proxyServerConn :: sendProxyMsg(void)
{
    string buf;
    pm_out.set_sequence(sequence++);
    pm_out.SerializeToString(&buf);
    const WebAppMessage wamPM(WS_TYPE_BINARY, buf);
    WaitUtil::Lock  lock(&sendLock);
    sendMessage(wamPM);
}


// below this line implements WebAppConnection


/*virtual*/
void
proxyServerConn :: onConnect(void)
{
    cout << "PFK wac onConnect" << endl;

    pm_out.Clear();
    pm_out.set_type(PMT_PROTOVERSION);
    pm_out.mutable_protover()->set_version(PMT_PROTO_VERSION_NUMBER);
    sendProxyMsg();
}

/*virtual*/
void
proxyServerConn :: onDisconnect(void)
{
    cout << "PFK wac onDisconnect" << endl;
    //xxx
}

/*virtual*/
bool
proxyServerConn :: onMessage(const WebAppServer::WebAppMessage &m)
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
        {
            // it's okay to start the proxy here.
            ret = startConnection();
        }
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
            cc = ::write(fd,
                         pm_in.data().data().c_str(),
                         pm_in.data().data().length());
            if (cc != pm_in.data().data().length())
                printf("PFK ERROR write %d != %d\n",
                       cc, pm_in.data().data().length());
        }
        if (cc <= 0)
            ret = false;
        break;
    }
    }

    return ret;
}

/*virtual*/
bool
proxyServerConn :: wacDoPoll(void)
{
    cout << "PFK wac poll"<< endl;
    return true;
}


// below this line implements fdThreadLauncher


/*virtual*/
bool
proxyServerConn :: doSelect(bool *forRead, bool *forWrite)
{
    *forRead = true;
    *forWrite = false;
    return true;
}

/*virtual*/
bool
proxyServerConn :: handleReadSelect(int fd)
{
    char buf[READ_BUFFER_SIZE];
    int cc = ::read(fd, buf, sizeof(buf));
    pm_out.Clear();
    if (cc <= 0)
    {
        pm_out.set_type(PMT_CLOSING);
        pm_out.mutable_closing()->set_reason("tcp proxy rcvd close");
        sendProxyMsg();
        return false;
    }
    pm_out.set_type(PMT_DATA);
    pm_out.mutable_data()->set_data(buf, cc);
    sendProxyMsg();
    return true;
}

/*virtual*/
bool
proxyServerConn :: handleWriteSelect(int fd)
{
    // xxx
    return false;
}

/*virtual*/
bool
proxyServerConn :: fdtlDoPoll(void)
{
//    cout << "PFK fdtl poll" << endl;
    return true;
}

/*virtual*/
void
proxyServerConn :: done(void)
{
    // xxx
}
