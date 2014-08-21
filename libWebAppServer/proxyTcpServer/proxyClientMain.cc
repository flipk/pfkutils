
#include "proxyClientConn.h"
#include "proxyClientTcpAcceptor.h"

#define LISTEN_PORT 2222

using namespace std;
using namespace WebAppServer;
using namespace WebAppClient;

int
main()
{
    fd_mgr   mgr(false);
    mgr.register_fd(new proxyClientTcpAcceptor(LISTEN_PORT));
    mgr.loop();
    return 0;
}
