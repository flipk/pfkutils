
#include "proxyClientConn.h"

//#define MY_PROXY "wwwgate0.mot.com:1080"
//#define MY_PROXY "10.0.0.15:1080"
//#define PROXY_CONNECT true
//#define PROXY_CONNECT false
#define MY_URL "ws://10.0.0.3:1081/websocket/test"
//#define MY_URL "ws://leviathan.phillipknaack.com/websocket/levproto"

using namespace std;
using namespace WebAppServer;
using namespace WebAppClient;

int
main()
{
#ifdef MY_PROXY
    proxyClientConn   client(MY_PROXY, MY_URL, PROXY_CONNECT);
#else
    proxyClientConn   client(MY_URL);
#endif

    while (1)
    {
        sleep(1);
        if (client.checkFinished())
            break;
    }

    return 0;
}
