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

#include "proxyServerConn.h"

#include <stdio.h>
#include <iostream>
#include <errno.h>
#include <sys/select.h>

using namespace std;
using namespace proxyTcp;
using namespace WebAppServer;

proxyServerConn :: proxyServerConn(const std::string &_destHost, int _destPort)
    : destHost(_destHost), destPort(_destPort)
{
    sequence = 1;
}

proxyServerConn :: ~proxyServerConn(void) ALLOW_THROWS
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
    sendMessage(wamPM);
}


// below this line implements WebAppConnection


/*virtual*/
void
proxyServerConn :: onConnect(void)
{
    cout << "PFK wac onConnect" << endl;

    WaitUtil::Lock  lock(&sendLock);
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
        WaitUtil::Lock  lock(&sendLock);
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
            WaitUtil::Lock  lock(&sendLock);
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
            uint8_t * ptr = (uint8_t *) pm_in.data().data().c_str();
            size_t bytes_left = (size_t) pm_in.data().data().length();
            while (bytes_left > 0)
            {
                cc = ::write(fd, ptr, bytes_left);
                if (cc == 0)
                {
                    printf("NOTE: write failed, end of file\n");
                    break;
                }
                if (cc < 0)
                {
                    int e = errno;
                    if (e != EAGAIN)
                    {
                        printf("NOTE: write failed! (cc=%d, %d:%s)\n",
                               cc, e, strerror(e));
                        break;
                    }
                }
                //else cc > 0
                if (cc != bytes_left)
                {
                    printf("NOTE : write of %d != %d\n", bytes_left, cc);
                    fd_set rfds;
                    FD_ZERO(&rfds);
                    FD_SET(fd, &rfds);
                    struct timeval tv;
                    tv.tv_sec = 1;
                    tv.tv_usec = 0;
                    select(fd+1,&rfds,NULL,NULL,&tv);
                }
                bytes_left -= cc;
                ptr += cc;
            }
        }
        if (cc <= 0)
            ret = false;
        break;
    }

    case PMT_PING:
    {
        WaitUtil::Lock  lock(&sendLock);
        pm_out.Clear();
        pm_out.set_type(PMT_PING);
        pm_out.mutable_ping()->CopyFrom(pm_in.ping());
        sendProxyMsg();
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
    WaitUtil::Lock  lock(&sendLock);
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
    // not used
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
