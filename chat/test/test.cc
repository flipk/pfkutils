#if 0
set -e -x
cd /home/flipk/proj/pfkutils/libWebAppServer
make -j 8
cd /home/flipk/proj/pfkutils/chat/test
g++ -g3 -c msgs.pb.cc -I/home/flipk/proj/protobuf/installed/include/
g++ -g3 -c test.cc -I ../../libWebAppServer -I/home/flipk/proj/protobuf/installed/include/
g++ -g3 msgs.pb.o test.o ../../libWebAppServer/libWebAppServer.a /home/flipk/proj/protobuf/installed/lib/libprotobuf.a -lpthread -o t
exit 0
#endif

#include "WebAppServer.h"
#include "base64.h"
#include "msgs.pb.h"

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
    /*virtual*/ bool onMessage(const WebAppServer::WebAppMessage &msg);
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

/*virtual*/ bool
testCallbackConn::onMessage(const WebAppServer::WebAppMessage &m)
{
    cout << "test.cc: testcallback msg" << endl;

    PFK::TestMsgs::Command_m  cmdMsg;

    if (cmdMsg.ParseFromString(m.buf) == false)
    {
        cout << "ParseFromString failed" << endl;
        return false;
    }

    switch (cmdMsg.type())
    {
    case PFK::TestMsgs::COMMAND_ADD:
    {        
        PFK::TestMsgs::Response_m  resp;
        resp.set_type(PFK::TestMsgs::RESPONSE_ADD);
        PFK::TestMsgs::ResponseAdd_m * ra = resp.mutable_add();
        ra->set_sum( cmdMsg.add().a() + cmdMsg.add().b() );
        std::string respStr;
        resp.SerializeToString( &respStr );
        const WebAppServer::WebAppMessage outm(
            WebAppServer::WS_TYPE_BINARY, respStr);
        sendMessage(outm);
        break;
    }
    }

    return true;
}
