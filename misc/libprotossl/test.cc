
#include "libprotossl.h"
#include "test_proto.pb.h"
#include "pfkposix.h"

#include <unistd.h>

using namespace ProtoSSL;
using namespace PFK::Test;

void
timeval_to_PingInfo(PingInfo &pi, const pfk_timeval &tv)
{
    pi.set_time_seconds(tv.tv_sec);
    pi.set_time_useconds(tv.tv_usec);
}

void
PingInfo_to_timeval(pfk_timeval &tv, const PingInfo &pi)
{
    tv.tv_sec = pi.time_seconds();
    tv.tv_usec = pi.time_useconds();
}

//
// server
//

class myConnServer : public ProtoSSLConn<ClientToServer, ServerToClient>
{
public:
    myConnServer(void) {
        printf("myConnServer::myConnServer\n");
    }
    ~myConnServer(void) {
        printf("myConnServer::~myConnServer\n");
    }
    void handleConnect(void)  {
        printf("myConnServer::handleConnect\n");
        outMessage().set_type(STC_PROTO_VERSION);
        outMessage().mutable_proto_version()->set_app_name("LIBPROTOSSL_TEST");
        outMessage().mutable_proto_version()->set_version(PROTOCOL_VERSION_3);
        sendMessage();
    }
    bool messageHandler(const ClientToServer &inMsg) {
        switch (inMsg.type())
        {
        case CTS_PROTO_VERSION:
            printf("server got proto app %s version %d from client\n",
                   inMsg.proto_version().app_name().c_str(),
                   inMsg.proto_version().version());
            break;
        case CTS_PING:
        {
            pfk_timeval ts;
            uint32_t seq = inMsg.ping().seq();
            PingInfo_to_timeval(ts, inMsg.ping());
            outMessage().set_type(STC_PING_ACK);
            outMessage().mutable_ping()->set_seq(seq);
            timeval_to_PingInfo(*outMessage().mutable_ping(),ts);
            sendMessage();
            break;
        }
        default:
            printf("server got unknown message %d\n",
                   inMsg.type());
        }
        return true;
    }
};

class myFactoryServer : public ProtoSSLConnFactory
{
public:
    myFactoryServer(void) { }
    ~myFactoryServer(void) { }
    _ProtoSSLConn * newConnection(void) {
        return new myConnServer;
    }
};

//
// client
//

class myConnClient : public ProtoSSLConn<ServerToClient, ClientToServer>
{
public:
    myConnClient(void) {
        printf("myConnClient::myConnClient\n");
    }
    ~myConnClient(void) {
        printf("myConnClient::~myConnClient\n");
    }
    void handleConnect(void)  {
        printf("myConnClient::handleConnect\n");
        outMessage().set_type(CTS_PROTO_VERSION);
        outMessage().mutable_proto_version()->set_app_name("LIBPROTOSSL_TEST");
        outMessage().mutable_proto_version()->set_version(PROTOCOL_VERSION_3);
        sendMessage();
    }
    bool messageHandler(const ServerToClient &inMsg) {
        bool done = false;
        switch (inMsg.type())
        {
        case STC_PROTO_VERSION:
        {
            pfk_timeval now;
            printf("client got proto app %s version %d from server\n",
                   inMsg.proto_version().app_name().c_str(),
                   inMsg.proto_version().version());
            now.getNow();
            outMessage().set_type(CTS_PING);
            outMessage().mutable_ping()->set_seq(1);
            timeval_to_PingInfo(*outMessage().mutable_ping(), now);
            sendMessage();
            break;
        }
        case STC_PING_ACK:
        {
            pfk_timeval ts, now, diff;
            uint32_t seq = inMsg.ping().seq();
            now.getNow();
            PingInfo_to_timeval(ts, inMsg.ping());
            diff = now - ts;
            printf("client got PING_ACK seq %d delay %u.%06u\n",
                   seq,
                   (unsigned int) diff.tv_sec,
                   (unsigned int) diff.tv_usec);
            if (seq < 10)
            {
                outMessage().set_type(CTS_PING);
                outMessage().mutable_ping()->set_seq(seq+1);
                timeval_to_PingInfo(*outMessage().mutable_ping(), now);
                sendMessage();
            }
            else
            {
                printf("successful test\n");
                stopMsgs();
                done = true;
            }
            break;
        }
        default:
            printf("client got unknown message %d\n",
                   inMsg.type());
        }
        return !done;
    }
};

class myFactoryClient : public ProtoSSLConnFactory
{
public:
    myFactoryClient(void) { }
    ~myFactoryClient(void) { }
    _ProtoSSLConn * newConnection(void) {
        return new myConnClient;
    }
};

//
// main
//

int
main(int argc, char ** argv)
{
    std::string cert_ca           = "file:keys/Root-CA.crt";

    std::string cert_server       = "file:keys/Server-Cert.crt";
    std::string key_server        = "file:keys/Server-Cert-encrypted.key";
    std::string key_pwd_server    = "0KZ7QMalU75s0IXoWnhm3BXEtswirfwrXwwNiF6c";
    std::string commonname_server = "Server Cert";

    std::string cert_client       = "file:keys/Client-Cert.crt";
    std::string key_client        = "file:keys/Client-Cert-encrypted.key";
    std::string key_pwd_client    = "IgiLNFWx3fTMioJycI8qXCep8j091yfHOwsBbo6f";
    std::string commonname_client = "Client Cert";

    if (argc != 2)
    {
        return 1;
    }
    std::string argv1(argv[1]);
    ProtoSSLMsgs  msgs;
    if (argv1 == "s")
    {
        ProtoSSLCertParams  certs(cert_ca,
                                  cert_server,
                                  key_server,
                                  key_pwd_server,
                                  commonname_client);
        myFactoryServer fact;

        if (msgs.loadCertificates(certs) == false)
            return 1;
    
        msgs.startServer(fact, 2005);
        while (msgs.run())
            ;
    }
    else if (argv1 == "c")
    {
        ProtoSSLCertParams  certs(cert_ca,
                                  cert_client,
                                  key_client,
                                  key_pwd_client,
                                  commonname_server);
        myFactoryClient fact;

        if (msgs.loadCertificates(certs) == false)
            return 1;
    
        msgs.startClient(fact, "104.131.232.148", 2005);
        while (msgs.run())
            ;
    }
    else
    {
        return 2;
    }

    return 0;
}
