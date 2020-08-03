
#include "libprotossl.h"
#include TEST_PROTO_HDR

using namespace PFK::Test;

class test_program
{
    ServerToClient stc;
    ClientToServer cts;
    bool connected;
    ProtoSSL::ProtoSSLMsgs  *msgs;
    ProtoSSL::ProtoSSLConnClient * client;
    ProtoSSL::ProtoSSLConnServer * server;
    ProtoSSL::ProtoSslDtlsQueueConfig dtlsq_config;
    ProtoSSL::ProtoSslDtlsQueue * dtlsq;
    uint32_t queue_number;
    pthread_t dtlsq_reader_id;
    bool dtlsq_reader_running;
    bool dtlsq_done;
    void usage(const char *progname)
    {
        printf("usage:\n"
               " be a server:\n"
               "   %s sP port\n"
               " be a client:\n"
               "   %s cP ipaddr port\n"
               " (where P is one of:\n"
               "   t : tcp TLS\n"
               "   u : udp DTLS\n"
               "   U : DTLSQUEUE\n", progname, progname);
        exit(1);
    }
public:
    test_program(void)
    {
        connected = false;
        client = NULL;
        server = NULL;
        msgs = NULL;
        dtlsq = NULL;
        queue_number = 0;
        dtlsq_reader_running = false;
        dtlsq_done = false;
    }
    ~test_program(void)
    {
        if (dtlsq)
        {
            dtlsq->shutdown();
            delete dtlsq;
            if (dtlsq_reader_running)
            {
                void * dummy = NULL;
                // the delete should cause ProtoSslDtlsQueue::handle_read
                // to return and the thread should exit on its own.
                pthread_join(dtlsq_reader_id, &dummy);
            }
        }
        if (server)
            delete server;
        if (client)
            delete client;
        if (msgs)
            delete msgs;
    }
    int main(int argc, char ** argv)
    {
        std::string cert_ca        = "file:keys/Root-CA.crt";
        std::string cert_server    = "file:keys/Server-Cert.crt";
        std::string key_server     = "file:keys/Server-Cert-plain.key";
        std::string cert_client    = "file:keys/Client-Cert.crt";
        std::string key_client     = "file:keys/Client-Cert-plain.key";

        if (argc < 2)
        {
            usage(argv[0]);
            //NOTREACHED
        }
        std::string argv1(argv[1]);
        if (argv1.size() != 2)
        {
            usage(argv[0]);
            //NOTREACHED
        }
        bool use_tcp = true;
        bool use_dtlsq = false;
        switch (argv1[1]) // the 'P' in the help msg
        {
        case 't':
            break;
        case 'u':
            use_tcp = false;
            break;
        case 'U':
            use_tcp = false;
            use_dtlsq = true;
            break;
        default:
            usage(argv[0]);
            //NOTREACHED
        }

        if (use_dtlsq)
        {
            // make the fragment size really small so we
            // exercise the fragmentation and reassembly code.
            dtlsq_config.fragment_size = 32;
            dtlsq_config.add_queue(
                queue_number,
                /*reliable*/ true,
                ProtoSSL::ProtoSslDtlsQueueConfig::FIFO,
                ProtoSSL::ProtoSslDtlsQueueConfig::NONE,
                /*limit*/ 0);
        }

        msgs = new ProtoSSL::ProtoSSLMsgs(
            /*nonblock*/true,
            /*debug*/false,
            /*read_timeout*/ 0,
            use_tcp,
            /*dtls_timeout_min*/ 1000,
            /*dtls_timeout_max*/ 2000 );

        if (argv1[0] == 's')
        {
            ProtoSSL::ProtoSSLCertParams  certs(cert_ca,
                                                cert_server,
                                                key_server,
                                                /*no password*/ "");

            if (msgs->loadCertificates(certs) == false)
                return 1;

            if (msgs->validateCertificates() == false)
                return 1;

            server = msgs->startServer(atoi(argv[2]));
            if (server == NULL)
            {
                fprintf(stderr,"failure to chooch making server\n");
                return 1;
            }
        }
        else if (argv1[0] == 'c')
        {
            if (argc != 4)
            {
                fprintf(stderr,"specify ip address of server\n");
                return 2;
            }

            ProtoSSL::ProtoSSLCertParams  certs(cert_ca,
                                                cert_client,
                                                key_client,
                                                /*no password*/ "");

            if (msgs->loadCertificates(certs) == false)
                return 1;

            if (msgs->validateCertificates() == false)
                return 1;

            client = msgs->startClient(argv[2], atoi(argv[3]));
            if (client == NULL)
            {
                fprintf(stderr,"failure to chooch making client\n");
                return 1;
            }
            connected = true;
            if (use_dtlsq)
            {
                dtlsq = new ProtoSSL::ProtoSslDtlsQueue(dtlsq_config,
                                                        client);
                if (dtlsq->ok())
                {
                    // lose the client pointer. dtlsq owns it now.
                    client = NULL;
                }
                else
                {
                    dtlsq->shutdown();
                    delete dtlsq;
                    dtlsq = NULL;
                    fprintf(stderr,"failure to chooch making dtlsq\n");
                    return 1;
                }
                start_dtlsq_reader();
            }
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

            if (use_dtlsq && dtlsq_done)
                break;
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
                    if (client || dtlsq)
                    {
                        fprintf(stderr,"rejecting new connection\n");
                        delete newclient;
                    }
                    else
                    {
                        fprintf(stderr,"got connection!\n");
                        if (use_dtlsq)
                        {
                            dtlsq = new ProtoSSL::ProtoSslDtlsQueue(
                                dtlsq_config, newclient);
                            if (dtlsq->ok())
                            {
                                start_dtlsq_reader();
                            }
                            else
                            {
                                dtlsq->shutdown();
                                delete dtlsq;
                                dtlsq = NULL;
                                fprintf(stderr,
                                        "failure to chooch making dtlsq\n");
                            }
                        }
                        else
                        {
                            client = newclient;
                        }
                        if (client || dtlsq)
                        {
                            connected = true;
                            send_client_protoversion();
                        }
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
                        send_client_message(stc);
                    }
                    else
                    {
                        ClientToServer cts;
                        cts.set_type(CTS_DATA);
                        cts.mutable_data()->set_data(buffer);
                        send_client_message(cts);
                    }
                }
                else
                    done = true;
            }
        }

        return 0;
    }
private:
    void start_dtlsq_reader(void)
    {
        pthread_create(&dtlsq_reader_id,
                       /*attr*/ NULL, &_dtlsq_reader_thread,
                       (void*) this);
    }

    static void * _dtlsq_reader_thread(void*arg)
    {
        test_program * tp = (test_program *) arg;
        tp->dtlsq_reader_thread();
        return NULL;
    }

    void dtlsq_reader_thread(void)
    {
        dtlsq_reader_running = true;
        bool done = false;
        while (!done)
        {
            ProtoSSL::ProtoSslDtlsQueue::read_return_t ret;
            if (server)
                ret = dtlsq->handle_read(cts);
            else
                ret = dtlsq->handle_read(stc);
            switch (ret)
            {
            case ProtoSSL::ProtoSslDtlsQueue::GOT_DISCONNECT:
                fprintf(stderr, "dtlsq read DISCONNECT\n");
                done = true;
                break;
            case ProtoSSL::ProtoSslDtlsQueue::READ_MORE:
                /* nothing to do, go round again */
                break;
            case ProtoSSL::ProtoSslDtlsQueue::GOT_TIMEOUT:
                /* wtf to do here ? elect nothing for now. */
                fprintf(stderr, "dtlsq read TIMEOUT?\n");
                break;
            case ProtoSSL::ProtoSslDtlsQueue::GOT_MESSAGE:
                if (server)
                    done = handle_client_msg();
                else
                    done = handle_server_msg();
                break;
            case ProtoSSL::ProtoSslDtlsQueue::LINK_DOWN:
                fprintf(stderr, "dtlsq LINK-DOWN!\n");
                break;
            case ProtoSSL::ProtoSslDtlsQueue::LINK_UP:
                fprintf(stderr, "dtlsq LINK-UP!\n");
                break;
            }
        }
        dtlsq_reader_running = false;
        connected = false;
        dtlsq_done = true;
    }

    void send_server_protoversion(void)
    {
        cts.Clear();
        cts.set_type(CTS_PROTO_VERSION);
        cts.mutable_proto_version()->set_app_name("LIBPROTOSSL_TEST2");
        cts.mutable_proto_version()->set_version(PROTOCOL_VERSION_3);
        send_client_message(cts);
    }

    void send_client_protoversion(void)
    {
        ServerToClient stc;
        stc.set_type(STC_PROTO_VERSION);
        stc.mutable_proto_version()->set_app_name("LIBPROTOSSL_TEST2");
        stc.mutable_proto_version()->set_version(PROTOCOL_VERSION_3);
        send_client_message(stc);
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
        send_client_message(cts);
    }

    void send_client_message(ProtoSSL::MESSAGE &msg)
    {
        if (client)
            client->send_message(msg);
        if (dtlsq)
            dtlsq->send_message(queue_number, msg);
        msg.Clear();
    }

    void send_client_ping_ack(void)
    {
        stc.set_type(STC_PING_ACK);
        stc.mutable_ping()->CopyFrom(*cts.mutable_ping());
        send_client_message(stc);
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
