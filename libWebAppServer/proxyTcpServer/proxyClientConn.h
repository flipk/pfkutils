/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*-  */
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

#include "WebSocketClient.h"
#include "fd_mgr.h"
#include "posix_fe.h"
#include "proxyProtos-proxyMsgs.pb.h"
#include <sys/time.h>

class proxyClientConn : public fd_interface,
                        public WebAppClient::WebSocketClient
{
public:
    proxyClientConn(const std::string &proxy, const std::string &url,
                    bool withConnect, int new_fd);
private:
    static const int READ_BUFFER_SIZE = 4096;

    /*virtual*/ ~proxyClientConn(void);
    bool allowReads;
    int sequence;
    pxfe_timeval lastPing;
    proxyTcp::ProxyMsg  pm_in;
    proxyTcp::ProxyMsg  pm_out;
    WaitUtil::Lockable  sendLock;
    void sendProxyMsg(void);

    // fd_interface

    /*virtual*/ rw_response read ( fd_mgr * );
    /*virtual*/ void select_rw ( fd_mgr *, bool * do_read, bool * do_write );

    // WebSocketClient

    /*virtual*/ void onConnect(void);
    /*virtual*/ void onDisconnect(void);
    /*virtual*/ bool onMessage(const WebAppServer::WebAppMessage &m);
};
