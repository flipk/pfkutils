
#include "libprotossl.h"
#include TEST_PROTO_HDR

using namespace PFK::Test;

class test_program
{
    ServerToClient stc;
    ClientToServer cts;
    bool connected;
    ProtoSSL::ProtoSSLMsgs  msgs;
    ProtoSSL::ProtoSSLConnClient * client;
    ProtoSSL::ProtoSSLConnServer * server;
public:
    test_program(void) : msgs(/*nonblock*/true, /*debugFlag*/false)
    {
        connected = false;
        client = NULL;
        server = NULL;
    }
    int main(int argc, char ** argv)
    {
        std::string cert_ca        = "file:keys/Root-CA.crt";

        std::string cert_server    = "file:keys/Server-Cert.crt";
        std::string key_server     = "file:keys/Server-Cert-encrypted.key";
        std::string key_pwd_server = "0KZ7QMalU75s0IXoWnhm3BXEtswirfwrXwwNiF6c";

        std::string cert_client    = "file:keys/Client-Cert.crt";
        std::string key_client     = "file:keys/Client-Cert-encrypted.key";
        std::string key_pwd_client = "IgiLNFWx3fTMioJycI8qXCep8j091yfHOwsBbo6f";

        if (argc < 2)
        {
            return 1;
        }
        std::string argv1(argv[1]);
        if (argv1 == "s")
        {
            ProtoSSL::ProtoSSLCertParams  certs(cert_ca,
                                                cert_server,
                                                key_server,
                                                key_pwd_server);

            if (msgs.loadCertificates(certs) == false)
                return 1;

            server = msgs.startServer(2005);
            if (server == NULL)
            {
                fprintf(stderr,"failure to chooch making server\n");
                return 1;
            }
        }
        else if (argv1 == "c")
        {
            if (argc != 3)
            {
                fprintf(stderr,"specify ip address of server\n");
                return 2;
            }

            ProtoSSL::ProtoSSLCertParams  certs(cert_ca,
                                                cert_client,
                                                key_client,
                                                key_pwd_client);

            if (msgs.loadCertificates(certs) == false)
                return 1;

            client = msgs.startClient(argv[2], 2005);
            if (client == NULL)
            {
                fprintf(stderr,"failure to chooch making client\n");
                return 1;
            }
            connected = true;
            send_server_protoversion();
        }
        else
        {
            return 2;
        }

        bool done = false;
        while (!done)
        {
            pxfe_select sel;

            if (server)
                sel.rfds.set(server->get_fd());
            if (client)
                sel.rfds.set(client->get_fd());
            if (connected)
                sel.rfds.set(0);
            sel.tv.set(1,0);

            sel.select();

            if (server && sel.rfds.is_set(server->get_fd()))
            {
                ProtoSSL::ProtoSSLConnClient * newclient =
                    server->handle_accept();

                if (newclient)
                {
                    if (client)
                    {
                        fprintf(stderr,"rejecting new connection\n");
                        delete newclient;
                    }
                    else
                    {
                        fprintf(stderr,"got connection!\n");
                        client = newclient;
                        connected = true;
                        send_client_protoversion();
                    }
                }
            }

            if (client && sel.rfds.is_set(client->get_fd()))
            {
                if (server)
                {
                    switch (client->handle_read(cts))
                    {
                    case ProtoSSL::ProtoSSLConnClient::GOT_DISCONNECT:
                        fprintf(stderr,"got disconnect\n");
                        done = true;
                        break;
                    case ProtoSSL::ProtoSSLConnClient::READ_MORE:
                        // nothing to do here
                        break;
                    case ProtoSSL::ProtoSSLConnClient::GOT_MESSAGE:
                        done = handle_client_msg();
                        break;
                    }
                }
                else
                {
                    switch (client->handle_read(stc))
                    {
                    case ProtoSSL::ProtoSSLConnClient::GOT_DISCONNECT:
                        fprintf(stderr,"got disconnect\n");
                        done = true;
                        break;
                    case ProtoSSL::ProtoSSLConnClient::READ_MORE:
                        // nothing to do here
                        break;
                    case ProtoSSL::ProtoSSLConnClient::GOT_MESSAGE:
                        done = handle_server_msg();
                        break;
                    }
                }
            }

            if (sel.rfds.is_set(0))
            {
                pxfe_string  buffer;
                pxfe_errno   e;
                int cc = buffer.read(0, 1000, &e);
                if (cc < 0)
                {
                    std::cerr << e.Format() << std::endl;
                    done = true;
                }
                else if (cc > 0)
                {
                    if (server)
                    {
                        ServerToClient stc;
                        stc.set_type(STC_DATA);
                        stc.mutable_data()->set_data(buffer);
                        client->send_message(stc);
                        stc.Clear();
                    }
                    else
                    {
                        ClientToServer cts;
                        cts.set_type(CTS_DATA);
                        cts.mutable_data()->set_data(buffer);
                        client->send_message(cts);
                        cts.Clear();
                    }
                }
                else
                    done = true;
            }
        }

        return 0;
    }
private:
    void send_server_protoversion(void)
    {
        cts.Clear();
        cts.set_type(CTS_PROTO_VERSION);
        cts.mutable_proto_version()->set_app_name("LIBPROTOSSL_TEST2");
        cts.mutable_proto_version()->set_version(PROTOCOL_VERSION_3);
        client->send_message(cts);
        cts.Clear();
    }

    void send_client_protoversion(void)
    {
        ServerToClient stc;
        stc.set_type(STC_PROTO_VERSION);
        stc.mutable_proto_version()->set_app_name("LIBPROTOSSL_TEST2");
        stc.mutable_proto_version()->set_version(PROTOCOL_VERSION_3);
        client->send_message(stc);
        stc.Clear();
    }

    void timeval_to_PingInfo(PingInfo &pi, const pxfe_timeval &tv)
    {
        pi.set_time_seconds (tv.tv_sec );
        pi.set_time_useconds(tv.tv_usec);
    }

    void PingInfo_to_timeval(pxfe_timeval &tv, const PingInfo &pi)
    {
        tv.tv_sec  = pi.time_seconds ();
        tv.tv_usec = pi.time_useconds();
    }

    bool handle_server_msg(void)
    {
        fprintf(stderr,"got msg from server: %s\n", stc.DebugString().c_str());
        switch (stc.type())
        {
        case STC_PROTO_VERSION:
            send_server_ping(1);
            break;
        case STC_PING_ACK:
            handle_ping_ack();
            break;
        }
        return false;
    }

    bool handle_client_msg(void)
    {
        fprintf(stderr,"got msg from client: %s\n", cts.DebugString().c_str());
        switch (cts.type())
        {
        case CTS_PING:
            send_client_ping_ack();
            break;
        }
        return false;
    }

    void send_server_ping(int seq)
    {
        pxfe_timeval now;
        cts.set_type(CTS_PING);
        cts.mutable_ping()->set_seq(seq);
        now.getNow();
        timeval_to_PingInfo(*cts.mutable_ping(), now);
        client->send_message(cts);
        cts.Clear();
    }

    void send_client_ping_ack(void)
    {
        stc.set_type(STC_PING_ACK);
        stc.mutable_ping()->CopyFrom(*cts.mutable_ping());
        client->send_message(stc);
        stc.Clear();
    }

    void handle_ping_ack(void)
    {
        pxfe_timeval ts, now, diff;
        uint32_t seq = stc.mutable_ping()->seq();
        now.getNow();
        PingInfo_to_timeval(ts, *stc.mutable_ping());
        diff = now - ts;
        fprintf(stderr,"client got PING_ACK seq %d delay %u.%06u\n",
               seq,
               (unsigned int) diff.tv_sec,
               (unsigned int) diff.tv_usec);
        if (seq < 3)
            send_server_ping(seq+1);
    }

};


int main(int argc, char ** argv)
{
    test_program tp;
    return tp.main(argc, argv);
}
