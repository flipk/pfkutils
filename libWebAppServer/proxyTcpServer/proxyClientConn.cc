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

#include "proxyClientConn.h"

#include <errno.h>

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
        done();
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
    *do_read = allowReads;
    *do_write = false;

    if (allowReads)
    {
        pxfe_timeval now, diff;
        gettimeofday(&now, NULL);
        diff = now - lastPing;
        if (diff.tv_sec > 10)
        {
            lastPing = now;
            WaitUtil::Lock  lock(&sendLock);
            pm_out.Clear();
            pm_out.set_type(PMT_PING);
            pm_out.mutable_ping()->set_time_sec( now.tv_sec );
            pm_out.mutable_ping()->set_time_usec( now.tv_usec );
            sendProxyMsg();
        }
    }
}

void
proxyClientConn :: sendProxyMsg(void)
{
    string buf;
    pm_out.set_sequence(sequence++);
    pm_out.SerializeToString(&buf);
    const WebAppMessage wamPM(WS_TYPE_BINARY, buf);
    sendMessage(wamPM);
}

/*virtual*/ void
proxyClientConn :: onConnect(void)
{
    ProxyMsg  pm;

    printf("PFK client onConnect\n");

    WaitUtil::Lock  lock(&sendLock);
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
            allowReads = true;
            gettimeofday(&lastPing, NULL);
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
                cc = ::write(fd_interface::fd, ptr, bytes_left);
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
                if (cc != (int) bytes_left)
                {
                    printf("NOTE: write of %u != %d\n",
                           (unsigned int) bytes_left, cc);
                    fd_set rfds;
                    FD_ZERO(&rfds);
                    FD_SET(fd_interface::fd, &rfds);
                    struct timeval tv;
                    tv.tv_sec = 1;
                    tv.tv_usec = 0;
                    select(fd_interface::fd+1,&rfds,NULL,NULL,&tv);
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
        pxfe_timeval  now, then, diff;
        gettimeofday(&now, NULL);
        then.tv_sec = pm_in.ping().time_sec();
        then.tv_usec = pm_in.ping().time_usec();
        diff = now - then;
        printf("ping round trip delay = %u.%06u\n",
               (unsigned int) diff.tv_sec, (unsigned int) diff.tv_usec);
        break;
    }
    }

    return ret;
}
