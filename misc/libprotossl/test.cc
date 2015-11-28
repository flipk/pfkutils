
#include "libprotossl.h"
#include "test_proto.pb.h"

#include <unistd.h>

using namespace ProtoSSL;
using namespace PFK::Test;

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
        outMessage().mutable_proto_version()->set_version(1);
        sendMessage();
    }
    bool messageHandler(const ClientToServer &inMsg) {
        printf("myConnServer::messageHandler\n");
        switch (inMsg.type())
        {
        case CTS_PROTO_VERSION:
            printf("server got proto version %d from client\n",
                   inMsg.proto_version().version());
            break;
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
        outMessage().mutable_proto_version()->set_version(1);
        sendMessage();
    }
    bool messageHandler(const ServerToClient &inMsg) {
        printf("myConnClient::messageHandler\n");
        switch (inMsg.type())
        {
        case STC_PROTO_VERSION:
            printf("client got proto version %d from server\n",
                   inMsg.proto_version().version());
            break;
        }
        sleep(1);
        stopMsgs();
        return false;
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
    
        msgs.startServer(fact, 5000);
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
    
        msgs.startClient(fact, "127.0.0.1", 5000);
        while (msgs.run())
            ;
    }
    else
    {
        return 2;
    }

    return 0;
}
