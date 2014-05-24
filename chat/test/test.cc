#if 0
set -e -x
cd /home/flipk/proj/pfkutils/libWebAppServer
make -j 8
cd /home/flipk/proj/pfkutils/chat/test
g++ -g3 -c test.cc -I ../../libWebAppServer/
g++ -g3 test.o ../../libWebAppServer/libWebAppServer.a -lpthread -o t
exit 0
#endif

#include "WebAppServer.h"

#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

using namespace std;

class testCallbackConn : public WebAppServer::WebAppConnection {
public:
    testCallbackConn(void) {
        cout << "test.cc: new conn" << endl;
    }
    virtual ~testCallbackConn(void) {
        cout << "test.cc: conn deleted" << endl;
    }
    /*virtual*/ bool onMessage(const WebAppServer::WebAppMessage &msg) {
        cout << "test.cc: testcallback msg" << endl;
        sendMessage(msg);
        return true;
    }
    /*virtual*/ bool doPoll(void) {
        cout  << "test.cc: testCallback poll" << endl;
        return true;
    }

};

class testCallback : public WebAppServer::WebAppConnectionCallback {
public:
    /*virtual*/ WebAppServer::WebAppConnection * newConnection(void)
    {
        return new testCallbackConn;
    }
};

int
main()
{
    srandom(getpid() * time(NULL));

    testCallback    callback;
    WebAppServer::WebAppServerConfig  serverConfig;
    WebAppServer::WebAppServer  server;

    serverConfig.addFastCGI(1083, "/cgi/test.cgi", &callback, 1000);

    server.start(&serverConfig);

    while (1)
        sleep(1);

    return 0;
}
