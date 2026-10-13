
#include "libprotossl.h"
#include TEST_PROTO_HDR
#include <inttypes.h>
#include <unistd.h>
#include <time.h>

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

    bool stress_mode;
    uint32_t stress_num_msgs;
    uint32_t queue_ctrl;
    uint32_t queue_bulk;
    bool stress_done;
    bool stress_success;
    bool senders_started;
    pthread_t ctrl_sender_id;
    pthread_t bulk_sender_id;
    bool ctrl_sender_running;
    bool bulk_sender_running;

    uint32_t expected_ctrl_seq;
    uint32_t expected_bulk_seq;
    bool server_ctrl_done;
    bool server_bulk_done;
    uint64_t total_bytes_received;

    void usage(const char *progname)
    {
        printf("usage:\n"
               " be a server:\n"
               "   %s sP port [--stress [num_msgs]]\n"
               " be a client:\n"
               "   %s cP ipaddr port [--stress [num_msgs]]\n"
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

        stress_mode = false;
        stress_num_msgs = 500;
        queue_ctrl = 0;
        queue_bulk = 0;
        stress_done = false;
        stress_success = true;
        senders_started = false;
        ctrl_sender_running = false;
        bulk_sender_running = false;
        expected_ctrl_seq = 0;
        expected_bulk_seq = 0;
        server_ctrl_done = false;
        server_bulk_done = false;
        total_bytes_received = 0;
    }
    ~test_program(void)
    {
        if (ctrl_sender_running)
        {
            void * dummy = NULL;
            pthread_join(ctrl_sender_id, &dummy);
        }
        if (bulk_sender_running)
        {
            void * dummy = NULL;
            pthread_join(bulk_sender_id, &dummy);
        }
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

        int arg_idx = (argv1[0] == 's') ? 3 : 4;
        for (int i = arg_idx; i < argc; i++)
        {
            if (strcmp(argv[i], "--stress") == 0)
            {
                stress_mode = true;
                if (i + 1 < argc && argv[i+1][0] != '-')
                {
                    stress_num_msgs = (uint32_t) atoi(argv[++i]);
                }
            }
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

        if (stress_mode && !use_dtlsq)
        {
            fprintf(stderr, "--stress requires DTLSQUEUE ('U' mode)\n");
            return 1;
        }

        if (use_dtlsq)
        {
            if (stress_mode)
            {
                dtlsq_config.fragment_size = 64;
                dtlsq_config.window_size = 64;
                dtlsq_config.hearbeat_interval = 2;
                dtlsq_config.max_outstanding_acks = 4;
                dtlsq_config.add_queue(
                    queue_ctrl,
                    /*reliable*/ true,
                    ProtoSSL::ProtoSslDtlsQueueConfig::FIFO,
                    ProtoSSL::ProtoSslDtlsQueueConfig::MESSAGES,
                    /*limit*/ 50);
                dtlsq_config.add_queue(
                    queue_bulk,
                    /*reliable*/ true,
                    ProtoSSL::ProtoSslDtlsQueueConfig::FIFO,
                    ProtoSSL::ProtoSslDtlsQueueConfig::BYTES,
                    /*limit*/ 65536);
            }
            else
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
            if (argc < 4)
            {
                fprintf(stderr,"specify ip address and port of server\n");
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

        time_t start_time = time(NULL);
        bool done = false;
        while (!done)
        {
            pxfe_select sel;

            if (use_dtlsq && dtlsq_done)
                break;
            if (stress_mode && !server && stress_done)
            {
                // client received STC_STRESS_DONE: wait briefly to flush, then finish
                usleep(150000);
                break;
            }
            if (stress_mode && server && stress_done)
            {
                // server sent STC_STRESS_DONE: wait for client to disconnect cleanly
                if (dtlsq_done)
                    break;
            }
            if (stress_mode && (time(NULL) - start_time > 60))
            {
                fprintf(stderr, "TIMEOUT in stress test (> 60s)!\n");
                stress_success = false;
                break;
            }
            if (server)
                sel.rfds.set(server->get_fd());
            if (client)
                sel.rfds.set(client->get_fd());
            if (connected && !stress_mode)
                sel.rfds.set(0);
            sel.tv.set(0, 100000);

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

        if (use_dtlsq && dtlsq)
        {
            ProtoSSL::ProtoSslDtlsQueueStatistics stats;
            dtlsq->get_stats(&stats);
            fprintf(stderr, "\n--- DTLSQ STATS (%s) ---\n", server ? "SERVER" : "CLIENT");
            fprintf(stderr, "  bytes_sent:              %" PRIu64 "\n", stats.bytes_sent);
            fprintf(stderr, "  bytes_received:          %" PRIu64 "\n", stats.bytes_received);
            fprintf(stderr, "  frags_sent:              %" PRIu64 "\n", stats.frags_sent);
            fprintf(stderr, "  frags_received:          %" PRIu64 "\n", stats.frags_received);
            fprintf(stderr, "  frags_resent:            %" PRIu64 "\n", stats.frags_resent);
            fprintf(stderr, "  missing_frags_detected:  %" PRIu64 "\n", stats.missing_frags_detected);
            fprintf(stderr, "-------------------------\n\n");
        }

        return stress_success ? 0 : 1;
    }
private:
    static uint32_t calc_crc32(const std::string &data)
    {
        uint32_t crc = 0xFFFFFFFF;
        for (size_t i = 0; i < data.size(); i++)
        {
            crc ^= (uint8_t) data[i];
            for (int j = 0; j < 8; j++)
                crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
        }
        return ~crc;
    }

    static std::string make_payload(uint32_t seq, size_t len)
    {
        std::string s;
        s.resize(len);
        for (size_t i = 0; i < len; i++)
            s[i] = (char) ((seq + i * 31) & 0xFF);
        return s;
    }

    static void * _ctrl_sender_thread(void * arg)
    {
        test_program * tp = (test_program *) arg;
        tp->ctrl_sender_thread();
        return NULL;
    }

    static void * _bulk_sender_thread(void * arg)
    {
        test_program * tp = (test_program *) arg;
        tp->bulk_sender_thread();
        return NULL;
    }

    void start_stress_senders(void)
    {
        if (senders_started)
            return;
        senders_started = true;
        ctrl_sender_running = true;
        bulk_sender_running = true;
        pthread_create(&ctrl_sender_id, NULL, &_ctrl_sender_thread, (void*) this);
        pthread_create(&bulk_sender_id, NULL, &_bulk_sender_thread, (void*) this);
    }

    void ctrl_sender_thread(void)
    {
        fprintf(stderr, "CLIENT: starting CTRL sender (%u msgs)...\n", stress_num_msgs);
        for (uint32_t seq = 0; seq < stress_num_msgs && !stress_done; seq++)
        {
            ClientToServer msg;
            msg.set_type(CTS_STRESS_MSG);
            StressMsg * sm = msg.mutable_stress();
            sm->set_queue_id(queue_ctrl);
            sm->set_seq(seq);
            sm->set_total_msgs(stress_num_msgs);
            sm->set_is_done(seq == stress_num_msgs - 1);
            std::string payload = make_payload(seq, 32);
            sm->set_payload(payload);
            sm->set_crc(calc_crc32(payload));

            while (!stress_done)
            {
                ProtoSSL::ProtoSslDtlsQueue::send_return_t ret =
                    dtlsq->send_message(queue_ctrl, msg);
                if (ret == ProtoSSL::ProtoSslDtlsQueue::SEND_SUCCESS)
                    break;
                if (ret == ProtoSSL::ProtoSslDtlsQueue::CONN_SHUTDOWN)
                    return;
                usleep(1000);
            }
            usleep(2000);
        }
        fprintf(stderr, "CLIENT: CTRL sender finished.\n");
        ctrl_sender_running = false;
    }

    void bulk_sender_thread(void)
    {
        fprintf(stderr, "CLIENT: starting BULK sender (%u msgs, 1024B each)...\n", stress_num_msgs);
        for (uint32_t seq = 0; seq < stress_num_msgs && !stress_done; seq++)
        {
            ClientToServer msg;
            msg.set_type(CTS_STRESS_MSG);
            StressMsg * sm = msg.mutable_stress();
            sm->set_queue_id(queue_bulk);
            sm->set_seq(seq);
            sm->set_total_msgs(stress_num_msgs);
            sm->set_is_done(seq == stress_num_msgs - 1);
            std::string payload = make_payload(seq, 1024);
            sm->set_payload(payload);
            sm->set_crc(calc_crc32(payload));

            while (!stress_done)
            {
                ProtoSSL::ProtoSslDtlsQueue::send_return_t ret =
                    dtlsq->send_message(queue_bulk, msg);
                if (ret == ProtoSSL::ProtoSslDtlsQueue::SEND_SUCCESS)
                    break;
                if (ret == ProtoSSL::ProtoSslDtlsQueue::CONN_SHUTDOWN)
                    return;
                usleep(500);
            }
            if ((seq % 100) == 0 && seq > 0)
                fprintf(stderr, "CLIENT: BULK progress: %u/%u messages sent\n", seq, stress_num_msgs);
        }
        fprintf(stderr, "CLIENT: BULK sender finished.\n");
        bulk_sender_running = false;
    }

    void handle_stress_msg(const StressMsg &sm)
    {
        uint32_t q = sm.queue_id();
        uint32_t seq = sm.seq();

        if (q == queue_ctrl)
        {
            if (seq != expected_ctrl_seq)
            {
                fprintf(stderr, "SERVER ERROR: CTRL queue out of order! Expected seq %u, got %u\n",
                        expected_ctrl_seq, seq);
                stress_success = false;
            }
            expected_ctrl_seq = seq + 1;
            if (sm.is_done())
            {
                fprintf(stderr, "SERVER: CTRL queue received DONE (seq %u).\n", seq);
                server_ctrl_done = true;
            }
        }
        else if (q == queue_bulk)
        {
            if (seq != expected_bulk_seq)
            {
                fprintf(stderr, "SERVER ERROR: BULK queue out of order! Expected seq %u, got %u\n",
                        expected_bulk_seq, seq);
                stress_success = false;
            }
            expected_bulk_seq = seq + 1;
            if (sm.is_done())
            {
                fprintf(stderr, "SERVER: BULK queue received DONE (seq %u).\n", seq);
                server_bulk_done = true;
            }
        }
        else
        {
            fprintf(stderr, "SERVER ERROR: received message with unexpected queue_id %u\n", q);
            stress_success = false;
        }

        if (sm.has_payload() && sm.payload().size() > 0)
        {
            uint32_t crc = calc_crc32(sm.payload());
            if (crc != sm.crc())
            {
                fprintf(stderr, "SERVER ERROR: CRC mismatch on queue %u seq %u (expected 0x%08x, got 0x%08x)!\n",
                        q, seq, sm.crc(), crc);
                stress_success = false;
            }
            total_bytes_received += sm.payload().size();
        }

        if (server_ctrl_done && server_bulk_done && !stress_done)
        {
            fprintf(stderr, "SERVER: all CTRL (%u) and BULK (%u) messages verified! Total payload: %" PRIu64 " bytes.\n",
                    expected_ctrl_seq, expected_bulk_seq, total_bytes_received);
            ServerToClient done_msg;
            done_msg.set_type(STC_STRESS_DONE);
            dtlsq->send_message(queue_ctrl, done_msg);
            stress_done = true;
        }
    }

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
        if (stress_mode)
        {
            switch (stc.type())
            {
            case STC_PROTO_VERSION:
                start_stress_senders();
                break;
            case STC_STRESS_DONE:
                fprintf(stderr, "CLIENT: received STC_STRESS_DONE from server!\n");
                stress_done = true;
                return true;
            default:
                break;
            }
            return false;
        }

        fprintf(stderr,"got msg from server: %s\n", stc.DebugString().c_str());
        switch (stc.type())
        {
        case STC_PROTO_VERSION:
            send_server_ping(1);
            break;
        case STC_PING_ACK:
            handle_ping_ack();
            break;
        default:
            break;
        }
        return false;
    }

    bool handle_client_msg(void)
    {
        if (stress_mode)
        {
            switch (cts.type())
            {
            case CTS_PROTO_VERSION:
                break;
            case CTS_STRESS_MSG:
                handle_stress_msg(cts.stress());
                if (stress_done)
                    return true;
                break;
            default:
                break;
            }
            return false;
        }

        fprintf(stderr,"got msg from client: %s\n", cts.DebugString().c_str());
        switch (cts.type())
        {
        case CTS_PING:
            send_client_ping_ack();
            break;
        default:
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
        {
            uint32_t q = (stress_mode && queue_ctrl != 0) ? queue_ctrl : queue_number;
            dtlsq->send_message(q, msg);
        }
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
