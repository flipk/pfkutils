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

#include "proxyClientTcpAcceptor.h"
#include "proxyClientConn.h"
#include "fdThreadLauncher.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>

using namespace std;

proxyClientTcpAcceptor :: proxyClientTcpAcceptor(short listenPort,
                                                 std::string _proxy,
                                                 bool _proxyConnect,
                                                 std::string _url)
{
    proxy = _proxy;
    proxyConnect = _proxyConnect;
    url = _url;

    fd = fdThreadLauncher::makeListeningSocket( listenPort );

    make_nonblocking();
}

/*virtual*/
proxyClientTcpAcceptor :: ~proxyClientTcpAcceptor(void)
{
    close(fd);
}

/*virtual*/
fd_interface::rw_response
proxyClientTcpAcceptor :: read ( fd_mgr * mgr )
{
    struct sockaddr_in sa;
    socklen_t  salen;

    salen = sizeof( sa );
    int new_fd = accept4( fd, (struct sockaddr *)&sa, &salen, SOCK_CLOEXEC );

    if ( new_fd < 0 )
    {
        if ( errno != EAGAIN )
            cerr << "accept: " << strerror( errno ) << endl;
        return OK;
    }

    proxyClientConn * pcc = new proxyClientConn(proxy, url,
                                                proxyConnect, new_fd);

    mgr->register_fd(pcc);

    pcc->startClient();

    return OK;
}

/*virtual*/
void
proxyClientTcpAcceptor :: select_rw ( fd_mgr *mgr,
                                      bool * do_read, bool * do_write )
{
    *do_read = true;
    *do_write = false;
}
