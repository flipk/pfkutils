
#include "libprotossl.h"
#include "test_proto.pb.h"

#include <iostream>
#include <unistd.h>

int
main(int argc, char ** argv)
{
    if (argc != 2)
        return 1;
    if (argv[1][0] == 's')
    {
        ProtoSSL::ProtoSSLMsgs<PFK::Test::ClientToServer,
                               PFK::Test::ServerToClient>
            svr(ProtoSSL::ProtoSSLCertParams(
                    "ca.crt",
                    "srv.crt",
                    "srv.key.enc",
                    "nErQniLmoG",
                    "Clnt Common Name"),
                2005);

        if (!svr.isGood())
        {
            std::cerr << "protosslmsgs not good\n";
            return 1;
        }

        while (1)
        {
            ProtoSSL::ProtoSSLEvent<PFK::Test::ClientToServer>  evt;
            PFK::Test::ServerToClient stc;

            if (!svr.isGood())
            {
                std::cerr << "protosslmsgs not good\n";
                return 1;
            }
            svr.getEvent( evt, 1000 );
            switch (evt.type)
            {
            case ProtoSSL::PROTOSSL_CONNECT:
                std::cout << "connection on id " << evt.connectionId
                          << std::endl;
                stc.set_type(PFK::Test::STC_PROTO_VERSION);
                stc.mutable_proto_version()->set_version(1);
                svr.sendMessage(stc);
                stc.Clear();
                break;
            case ProtoSSL::PROTOSSL_DISCONNECT:
                std::cout << "disconnect id " << evt.connectionId << std::endl;
                break;
            case ProtoSSL::PROTOSSL_TIMEOUT:
                std::cout << "timeout" << std::endl;
                break;
            case ProtoSSL::PROTOSSL_MESSAGE:
                std::cout << "event on conn " << evt.connectionId << std::endl;
                switch (evt.msg->type())
                {
                case PFK::Test::CTS_PROTO_VERSION:
                    std::cout << "got proto version "
                              << evt.msg->proto_version().version() 
                              << std::endl;
                    break;
                default:
                    std::cout << "unknown msg rcvd" << std::endl;
                    break;
                }
                break;
            case ProtoSSL::PROTOSSL_RETRY:
                // do nothing
                break;
            }
        }
    }
    else if (argv[1][0] == 'c')
    {
        ProtoSSL::ProtoSSLMsgs<PFK::Test::ServerToClient,
                               PFK::Test::ClientToServer>
            clnt(ProtoSSL::ProtoSSLCertParams(
                    "ca.crt",
                    "clnt.crt",
                    "clnt.key.enc",
                    "nFunJ0ODB0",
                    "Srv Common Name"),
                 "127.0.0.1", 2005);

        if (!clnt.isGood())
        {
            std::cerr << "protosslmsgs is not good\n";
            return 1;
        }

        while (1)
        {
            ProtoSSL::ProtoSSLEvent<PFK::Test::ServerToClient>  evt;
            PFK::Test::ClientToServer  cts;

            if (!clnt.isGood())
            {
                std::cerr << "protosslmsgs is not good\n";
                return 1;
            }
            clnt.getEvent( evt, 1000 );
            switch (evt.type)
            {
            case ProtoSSL::PROTOSSL_CONNECT:
                std::cout << "connection on id " << evt.connectionId
                          << std::endl;
                cts.set_type(PFK::Test::CTS_PROTO_VERSION);
                cts.mutable_proto_version()->set_version(1);
                clnt.sendMessage(cts);
                cts.Clear();
                break;
            case ProtoSSL::PROTOSSL_DISCONNECT:
                std::cout << "disconnect id " << evt.connectionId << std::endl;
                break;
            case ProtoSSL::PROTOSSL_TIMEOUT:
                std::cout << "timeout" << std::endl;
                break;
            case ProtoSSL::PROTOSSL_MESSAGE:
                std::cout << "event on conn " << evt.connectionId << std::endl;
                switch (evt.msg->type())
                {
                case PFK::Test::CTS_PROTO_VERSION:
                    std::cout << "got proto version "
                              << evt.msg->proto_version().version() 
                              << std::endl;
                    break;
                default:
                    std::cout << "unknown msg rcvd" << std::endl;
                    break;
                }
                break;
            case ProtoSSL::PROTOSSL_RETRY:
                sleep(1);
                // do nothing
                break;
            }
        }

    }
    else
        return 2;

    return 0;
}
