
#include "proxyClientTcpAcceptor.h"
#include "proxyClientConn.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>

//#define MY_PROXY "wwwgate0.mot.com:1080"
//#define MY_PROXY "10.0.0.15:1080"
//#define PROXY_CONNECT true
//#define PROXY_CONNECT false
#define MY_URL "ws://10.0.0.3:1081/websocket/proxy"
//#define MY_URL "ws://leviathan.phillipknaack.com/websocket/levproto"

using namespace std;

proxyClientTcpAcceptor :: proxyClientTcpAcceptor(short port)
{
#ifdef MY_PROXY
    proxy = MY_PROXY;
#endif
#ifdef PROXY_CONNECT
    proxyConnect = PROXY_CONNECT;
#else
    proxyConnect = false;
#endif
    url = MY_URL;

    fd = socket( AF_INET, SOCK_STREAM, 0 );
    if ( fd < 0 )
    {
        cerr << "socket: " << strerror(errno) << endl;
        exit( 1 );
    }
    int v = 1;
    setsockopt( fd, SOL_SOCKET, SO_REUSEADDR, (void*) &v, sizeof( v ));
    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_port = htons( port );
    sa.sin_addr.s_addr = INADDR_ANY;
    if ( bind( fd, (struct sockaddr *)&sa, sizeof( sa )) < 0 )
    {
        cerr << "bind: " << strerror( errno ) << endl;
        exit( 1 );
    }
    listen( fd, 1 );
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
