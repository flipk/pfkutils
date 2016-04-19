
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
    int new_fd = accept( fd, (struct sockaddr *)&sa, &salen );

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
