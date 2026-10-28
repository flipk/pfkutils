#include "libprotossl.h"
#include TEST_PROTO_HDR
#include "posix_fe.h"
#include <inttypes.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <deque>
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>

using namespace PFK::Test;

// =========================================================================
// CRC32 and Payload Generation
// =========================================================================

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

static std::string find_cert_path(const std::string &filename)
{
    std::string p1 = "keys/" + filename;
    if (access(p1.c_str(), R_OK) == 0)
        return "file:" + p1;
    std::string p2 = "libprotossl/keys/" + filename;
    if (access(p2.c_str(), R_OK) == 0)
        return "file:" + p2;
    return "file:" + p1;
}

// =========================================================================
// Fault-Injecting UDP Proxy
// =========================================================================

struct FaultConfig {
    int drop_percent;           // 0..100
    int burst_drop_interval;    // drop every N packets (0 = disabled)
    int burst_drop_count;       // number of consecutive packets to drop in a burst
    int delay_ms;               // latency in milliseconds (0 = immediate)
    int duplicate_percent;      // duplicate packet chance 0..100
};

class UdpFaultProxy {
    FaultConfig cfg;
    short listen_port;
    short remote_port;
    uint32_t remote_ip;

    pxfe_udp_socket sock_client;
    pxfe_udp_socket sock_server;
    pxfe_pipe closer_pipe;
    pthread_t tid;
    bool running;

    struct DelayedPacket {
        pxfe_string buf;
        pxfe_sockaddr_in dest;
        bool is_c2s;
        uint64_t deliver_at_us;
    };
    std::deque<DelayedPacket> delay_q;

public:
    uint32_t packets_c2s;
    uint32_t packets_s2c;
    uint32_t packets_dropped;
    uint32_t packets_duplicated;
    uint32_t total_packets;

    UdpFaultProxy(void)
        : listen_port(0), remote_port(0), remote_ip(0),
          running(false), packets_c2s(0), packets_s2c(0),
          packets_dropped(0), packets_duplicated(0), total_packets(0)
    {
    }

    ~UdpFaultProxy(void)
    {
        stop();
    }

    bool start(short _listen_port, short _remote_port, const FaultConfig &_cfg)
    {
        cfg = _cfg;
        listen_port = _listen_port;
        remote_port = _remote_port;
        remote_ip = 0x7F000001; // 127.0.0.1

        packets_c2s = 0;
        packets_s2c = 0;
        packets_dropped = 0;
        packets_duplicated = 0;
        total_packets = 0;
        delay_q.clear();

        pxfe_errno e;
        if (!sock_client.init(listen_port, &e))
        {
            fprintf(stderr, "UdpFaultProxy: sock_client init(%d) failed: %s\n",
                    listen_port, e.Format().c_str());
            return false;
        }
        if (!sock_server.init(&e))
        {
            fprintf(stderr, "UdpFaultProxy: sock_server init failed: %s\n",
                    e.Format().c_str());
            sock_client.close();
            return false;
        }

        running = true;
        pthread_create(&tid, NULL, &_proxy_thread, this);
        return true;
    }

    void stop(void)
    {
        if (!running)
            return;
        char dummy = 1;
        if (::write(closer_pipe.writeEnd, &dummy, 1) < 0) { }
        pthread_join(tid, NULL);
        running = false;
        sock_client.close();
        sock_server.close();
    }

private:
    static void * _proxy_thread(void * arg)
    {
        ((UdpFaultProxy*)arg)->proxy_thread();
        return NULL;
    }

    bool should_drop(void)
    {
        total_packets++;
        if (cfg.burst_drop_interval > 0 && cfg.burst_drop_count > 0)
        {
            uint32_t phase = total_packets % cfg.burst_drop_interval;
            if (phase < (uint32_t)cfg.burst_drop_count)
            {
                packets_dropped++;
                return true;
            }
        }
        if (cfg.drop_percent > 0)
        {
            if ((rand() % 100) < cfg.drop_percent)
            {
                packets_dropped++;
                return true;
            }
        }
        return false;
    }

    bool should_duplicate(void)
    {
        if (cfg.duplicate_percent > 0)
        {
            if ((rand() % 100) < cfg.duplicate_percent)
            {
                packets_duplicated++;
                return true;
            }
        }
        return false;
    }

    void proxy_thread(void)
    {
        pxfe_sockaddr_in server_dest;
        server_dest.init(remote_ip, remote_port);

        pxfe_sockaddr_in client_source;
        bool have_client_source = false;

        pxfe_errno e;
        while (1)
        {
            pxfe_select sel;
            sel.rfds.set(closer_pipe.readEnd);
            sel.rfds.set(sock_client);
            sel.rfds.set(sock_server);

            pxfe_timeval now;
            now.getNow();
            uint64_t now_us = now.usecs();

            // deliver any delayed packets that are due
            while (!delay_q.empty() && delay_q.front().deliver_at_us <= now_us)
            {
                DelayedPacket &dp = delay_q.front();
                if (dp.is_c2s)
                {
                    pxfe_sockaddr_in dest;
                    dest.init(remote_ip, remote_port);
                    sock_server.send(dp.buf, dest, &e);
                }
                else
                {
                    sock_client.send(dp.buf, dp.dest, &e);
                }
                delay_q.pop_front();
            }

            if (!delay_q.empty())
            {
                uint64_t wait_us = 0;
                if (delay_q.front().deliver_at_us > now_us)
                    wait_us = delay_q.front().deliver_at_us - now_us;
                sel.tv.set(wait_us / 1000000, wait_us % 1000000);
            }
            else
            {
                sel.tv.set(0, 50000); // 50ms default timeout
            }

            sel.select();

            if (sel.rfds.is_set(closer_pipe.readEnd))
                break;

            // Packet from Client -> forward to Server
            if (sel.rfds.is_set(sock_client))
            {
                pxfe_string buf;
                if (sock_client.recv(buf, client_source, &e))
                {
                    have_client_source = true;
                    packets_c2s++;
                    if (!should_drop())
                    {
                        if (cfg.delay_ms > 0)
                        {
                            DelayedPacket dp;
                            dp.buf = buf;
                            dp.dest = server_dest;
                            dp.is_c2s = true;
                            dp.deliver_at_us = now_us + ((uint64_t)cfg.delay_ms * 1000);
                            delay_q.push_back(dp);
                        }
                        else
                        {
                            pxfe_sockaddr_in dest;
                            dest.init(remote_ip, remote_port);
                            sock_server.send(buf, dest, &e);
                            if (should_duplicate())
                                sock_server.send(buf, dest, &e);
                        }
                    }
                }
            }

            // Packet from Server -> forward to Client
            if (sel.rfds.is_set(sock_server))
            {
                pxfe_string buf;
                pxfe_sockaddr_in dummy;
                if (sock_server.recv(buf, dummy, &e))
                {
                    packets_s2c++;
                    if (have_client_source && !should_drop())
                    {
                        if (cfg.delay_ms > 0)
                        {
                            DelayedPacket dp;
                            dp.buf = buf;
                            dp.dest = client_source;
                            dp.is_c2s = false;
                            dp.deliver_at_us = now_us + ((uint64_t)cfg.delay_ms * 1000);
                            delay_q.push_back(dp);
                        }
                        else
                        {
                            sock_client.send(buf, client_source, &e);
                            if (should_duplicate())
                                sock_client.send(buf, client_source, &e);
                        }
                    }
                }
            }
        }
    }
};

// =========================================================================
// Scenario Definitions & Results
// =========================================================================

struct StressScenario {
    int id;
    std::string name;
    std::string description;
    FaultConfig fault;
    uint32_t fragment_size;
    uint32_t window_size;
    uint32_t num_ctrl_msgs;
    uint32_t num_bulk_msgs;
    uint32_t max_missed_heartbeats;
    bool expect_resends;
};

struct ScenarioResult {
    bool passed;
    double elapsed_sec;
    uint64_t bytes_sent;
    uint64_t bytes_received;
    uint64_t frags_sent;
    uint64_t frags_received;
    uint64_t frags_resent;
    uint64_t missing_frags;
    uint32_t packets_dropped;
    uint32_t packets_duplicated;
    std::string error_msg;
};

// =========================================================================
// Test Harness (Server + Client in coordinated execution)
// =========================================================================

class TestSession {
    const StressScenario &scen;
    short server_port;
    short proxy_port;
    bool verbose;

    UdpFaultProxy proxy;

    ProtoSSL::ProtoSSLMsgs *server_msgs;
    ProtoSSL::ProtoSSLMsgs *client_msgs;

    ProtoSSL::ProtoSSLConnServer *server_listener;
    ProtoSSL::ProtoSSLConnClient *server_conn;
    ProtoSSL::ProtoSSLConnClient *client_conn;

    ProtoSSL::ProtoSslDtlsQueue *server_dtlsq;
    ProtoSSL::ProtoSslDtlsQueue *client_dtlsq;

    uint32_t queue_ctrl;
    uint32_t queue_bulk;

    pthread_t server_reader_tid;
    pthread_t client_reader_tid;
    pthread_t ctrl_sender_tid;
    pthread_t bulk_sender_tid;

    bool server_reader_running;
    bool client_reader_running;
    bool ctrl_sender_running;
    bool bulk_sender_running;

    bool server_done;
    bool client_done;
    bool session_failed;
    std::string failure_reason;

    uint32_t expected_ctrl_seq;
    uint32_t expected_bulk_seq;
    bool server_ctrl_completed;
    bool server_bulk_completed;
    uint64_t total_payload_received;

    WaitUtil::Lockable lock;

public:
    TestSession(const StressScenario &_scen, short base_port, bool _verbose)
        : scen(_scen), server_port(base_port), proxy_port(base_port + 1), verbose(_verbose),
          server_msgs(NULL), client_msgs(NULL), server_listener(NULL),
          server_conn(NULL), client_conn(NULL), server_dtlsq(NULL), client_dtlsq(NULL),
          queue_ctrl(0), queue_bulk(0),
          server_reader_running(false), client_reader_running(false),
          ctrl_sender_running(false), bulk_sender_running(false),
          server_done(false), client_done(false), session_failed(false),
          expected_ctrl_seq(0), expected_bulk_seq(0),
          server_ctrl_completed(false), server_bulk_completed(false),
          total_payload_received(0)
    {
    }

    ~TestSession(void)
    {
        cleanup();
    }

    struct ServerAcceptData {
        ProtoSSL::ProtoSSLConnServer *listener;
        ProtoSSL::ProtoSSLConnClient *conn;
        bool stop;
    };

    static void * _server_accept_thread(void * arg)
    {
        ServerAcceptData * d = (ServerAcceptData*)arg;
        time_t start = time(NULL);
        while (!d->conn && !d->stop && (time(NULL) - start < 10))
        {
            pxfe_select sel;
            sel.rfds.set(d->listener->get_fd());
            sel.tv.set(0, 50000);
            sel.select();
            if (sel.rfds.is_set(d->listener->get_fd()))
                d->conn = d->listener->handle_accept();
        }
        return NULL;
    }

    ScenarioResult run(void)
    {
        ScenarioResult res;
        memset(&res, 0, sizeof(res));
        res.passed = false;

        pxfe_timeval start_tv, end_tv;
        start_tv.getNow();

        // 1. Start Fault Proxy
        if (!proxy.start(proxy_port, server_port, scen.fault))
        {
            res.error_msg = "Failed to bind proxy ports";
            return res;
        }

        // 2. Setup Server
        std::string ca_file   = find_cert_path("Root-CA.crt");
        std::string s_cert    = find_cert_path("Server-Cert.crt");
        std::string s_key     = find_cert_path("Server-Cert-plain.key");
        std::string c_cert    = find_cert_path("Client-Cert.crt");
        std::string c_key     = find_cert_path("Client-Cert-plain.key");

        ProtoSSL::ProtoSSLCertParams server_cert_params(ca_file, s_cert, s_key, "");
        server_msgs = new ProtoSSL::ProtoSSLMsgs(true, false, 0, false, 1000, 2000);
        if (!server_msgs->loadCertificates(server_cert_params) || !server_msgs->validateCertificates())
        {
            res.error_msg = "Server certificate validation failed";
            cleanup();
            return res;
        }
        server_listener = server_msgs->startServer(server_port);
        if (!server_listener)
        {
            res.error_msg = "Server failed to start on port";
            cleanup();
            return res;
        }

        // 3. Setup Client
        ProtoSSL::ProtoSSLCertParams client_cert_params(ca_file, c_cert, c_key, "");
        client_msgs = new ProtoSSL::ProtoSSLMsgs(true, false, 0, false, 1000, 2000);
        if (!client_msgs->loadCertificates(client_cert_params) || !client_msgs->validateCertificates())
        {
            res.error_msg = "Client certificate validation failed";
            cleanup();
            return res;
        }

        ServerAcceptData accept_data = { server_listener, NULL, false };
        pthread_t accept_tid;
        pthread_create(&accept_tid, NULL, &_server_accept_thread, &accept_data);

        client_conn = client_msgs->startClient("127.0.0.1", proxy_port);
        if (!client_conn)
        {
            accept_data.stop = true;
            pthread_join(accept_tid, NULL);
            if (accept_data.conn)
                delete accept_data.conn;
            res.error_msg = "Client failed to connect to proxy port";
            cleanup();
            return res;
        }

        pthread_join(accept_tid, NULL);
        server_conn = accept_data.conn;
        if (!server_conn)
        {
            res.error_msg = "Server accept timed out";
            cleanup();
            return res;
        }

        // 4. Configure Queues
        ProtoSSL::ProtoSslDtlsQueueConfig dtlsq_config;
        dtlsq_config.fragment_size = scen.fragment_size;
        dtlsq_config.window_size = scen.window_size;
        dtlsq_config.hearbeat_interval = 2;
        dtlsq_config.max_outstanding_acks = 4;
        dtlsq_config.add_queue(queue_ctrl, true, ProtoSSL::ProtoSslDtlsQueueConfig::FIFO,
                               ProtoSSL::ProtoSslDtlsQueueConfig::MESSAGES, 50);
        dtlsq_config.add_queue(queue_bulk, true, ProtoSSL::ProtoSslDtlsQueueConfig::FIFO,
                               ProtoSSL::ProtoSslDtlsQueueConfig::BYTES, 65536);

        // Client DTLSQ
        client_dtlsq = new ProtoSSL::ProtoSslDtlsQueue(dtlsq_config, client_conn);
        client_conn = NULL; // dtlsq takes ownership

        // Server DTLSQ
        server_dtlsq = new ProtoSSL::ProtoSslDtlsQueue(dtlsq_config, server_conn);
        server_conn = NULL; // dtlsq takes ownership

        // Start Reader Threads
        server_reader_running = true;
        pthread_create(&server_reader_tid, NULL, &_server_reader_thread, this);

        client_reader_running = true;
        pthread_create(&client_reader_tid, NULL, &_client_reader_thread, this);

        // Handshake: exchange protocol version
        ServerToClient stc_v;
        stc_v.set_type(STC_PROTO_VERSION);
        stc_v.mutable_proto_version()->set_app_name("DTLSQ_STRESS");
        stc_v.mutable_proto_version()->set_version(PROTOCOL_VERSION_3);
        server_dtlsq->send_message(queue_ctrl, stc_v);

        ClientToServer cts_v;
        cts_v.set_type(CTS_PROTO_VERSION);
        cts_v.mutable_proto_version()->set_app_name("DTLSQ_STRESS");
        cts_v.mutable_proto_version()->set_version(PROTOCOL_VERSION_3);
        client_dtlsq->send_message(queue_ctrl, cts_v);

        // Start Client Worker Senders
        ctrl_sender_running = true;
        pthread_create(&ctrl_sender_tid, NULL, &_ctrl_sender_thread, this);

        bulk_sender_running = true;
        pthread_create(&bulk_sender_tid, NULL, &_bulk_sender_thread, this);

        // Wait for completion with timeout (30 seconds)
        time_t t_start = time(NULL);
        while (!client_done && !session_failed && (time(NULL) - t_start < 30))
        {
            usleep(20000); // 20ms poll
        }

        if (session_failed)
        {
            res.error_msg = failure_reason;
        }
        else if (!client_done)
        {
            res.error_msg = "Scenario timed out (>30s)";
        }
        else
        {
            // Give a short grace period for server to see final ACKs
            usleep(100000);

            ProtoSSL::ProtoSslDtlsQueueStatistics client_stats;
            client_dtlsq->get_stats(&client_stats);

            res.bytes_sent = client_stats.bytes_sent;
            res.bytes_received = client_stats.bytes_received;
            res.frags_sent = client_stats.frags_sent;
            res.frags_received = client_stats.frags_received;
            res.frags_resent = client_stats.frags_resent;
            res.missing_frags = client_stats.missing_frags_detected;
            res.packets_dropped = proxy.packets_dropped;
            res.packets_duplicated = proxy.packets_duplicated;

            if (scen.expect_resends && res.frags_resent == 0)
            {
                res.error_msg = "Expected retransmissions but frags_resent was 0";
            }
            else
            {
                res.passed = true;
            }
        }

        end_tv.getNow();
        pxfe_timeval diff = end_tv - start_tv;
        res.elapsed_sec = diff.tv_sec + (diff.tv_usec / 1000000.0);

        cleanup();
        return res;
    }

private:
    void cleanup(void)
    {
        client_done = true;
        server_done = true;

        if (ctrl_sender_running)
        {
            pthread_join(ctrl_sender_tid, NULL);
            ctrl_sender_running = false;
        }
        if (bulk_sender_running)
        {
            pthread_join(bulk_sender_tid, NULL);
            bulk_sender_running = false;
        }

        if (client_dtlsq)
            client_dtlsq->shutdown();
        if (client_reader_running)
        {
            pthread_join(client_reader_tid, NULL);
            client_reader_running = false;
        }
        if (client_dtlsq)
        {
            delete client_dtlsq;
            client_dtlsq = NULL;
        }

        if (server_dtlsq)
            server_dtlsq->shutdown();
        if (server_reader_running)
        {
            pthread_join(server_reader_tid, NULL);
            server_reader_running = false;
        }
        if (server_dtlsq)
        {
            delete server_dtlsq;
            server_dtlsq = NULL;
        }

        proxy.stop();

        if (server_listener)
        {
            delete server_listener;
            server_listener = NULL;
        }
        if (client_msgs)
        {
            delete client_msgs;
            client_msgs = NULL;
        }
        if (server_msgs)
        {
            delete server_msgs;
            server_msgs = NULL;
        }
    }

    static void * _server_reader_thread(void * arg)
    {
        ((TestSession*)arg)->server_reader();
        return NULL;
    }

    void server_reader(void)
    {
        ClientToServer cts;
        while (!server_done)
        {
            ProtoSSL::ProtoSslDtlsQueue::read_return_t ret = server_dtlsq->handle_read(cts);
            if (ret == ProtoSSL::ProtoSslDtlsQueue::GOT_MESSAGE)
            {
                if (cts.type() == CTS_STRESS_MSG && cts.has_stress())
                {
                    const StressMsg &sm = cts.stress();
                    uint32_t q = sm.queue_id();
                    uint32_t seq = sm.seq();

                    if (q == queue_ctrl)
                    {
                        if (seq != expected_ctrl_seq)
                        {
                            failure("Server CTRL queue out of order: expected " +
                                    std::to_string(expected_ctrl_seq) + ", got " + std::to_string(seq));
                        }
                        expected_ctrl_seq++;
                        if (sm.is_done())
                            server_ctrl_completed = true;
                    }
                    else if (q == queue_bulk)
                    {
                        if (seq != expected_bulk_seq)
                        {
                            failure("Server BULK queue out of order: expected " +
                                    std::to_string(expected_bulk_seq) + ", got " + std::to_string(seq));
                        }
                        expected_bulk_seq++;
                        if (sm.is_done())
                            server_bulk_completed = true;
                    }

                    if (sm.has_payload() && sm.payload().size() > 0)
                    {
                        uint32_t crc = calc_crc32(sm.payload());
                        if (crc != sm.crc())
                        {
                            failure("Server CRC mismatch on queue " + std::to_string(q) +
                                    " seq " + std::to_string(seq));
                        }
                        total_payload_received += sm.payload().size();
                    }

                    if (server_ctrl_completed && server_bulk_completed)
                    {
                        ServerToClient done_msg;
                        done_msg.set_type(STC_STRESS_DONE);
                        server_dtlsq->send_message(queue_ctrl, done_msg);
                        break;
                    }
                }
            }
            else if (ret == ProtoSSL::ProtoSslDtlsQueue::GOT_DISCONNECT)
            {
                break;
            }
        }
    }

    static void * _client_reader_thread(void * arg)
    {
        ((TestSession*)arg)->client_reader();
        return NULL;
    }

    void client_reader(void)
    {
        ServerToClient stc;
        while (!client_done)
        {
            ProtoSSL::ProtoSslDtlsQueue::read_return_t ret = client_dtlsq->handle_read(stc);
            if (ret == ProtoSSL::ProtoSslDtlsQueue::GOT_MESSAGE)
            {
                if (stc.type() == STC_STRESS_DONE)
                {
                    client_done = true;
                    break;
                }
            }
            else if (ret == ProtoSSL::ProtoSslDtlsQueue::GOT_DISCONNECT)
            {
                break;
            }
        }
    }

    static void * _ctrl_sender_thread(void * arg)
    {
        ((TestSession*)arg)->ctrl_sender();
        return NULL;
    }

    void ctrl_sender(void)
    {
        for (uint32_t seq = 0; seq < scen.num_ctrl_msgs && !client_done && !session_failed; seq++)
        {
            ClientToServer msg;
            msg.set_type(CTS_STRESS_MSG);
            StressMsg * sm = msg.mutable_stress();
            sm->set_queue_id(queue_ctrl);
            sm->set_seq(seq);
            sm->set_total_msgs(scen.num_ctrl_msgs);
            sm->set_is_done(seq == scen.num_ctrl_msgs - 1);
            std::string payload = make_payload(seq, 32);
            sm->set_payload(payload);
            sm->set_crc(calc_crc32(payload));

            while (!client_done && !session_failed)
            {
                ProtoSSL::ProtoSslDtlsQueue::send_return_t ret =
                    client_dtlsq->send_message(queue_ctrl, msg);
                if (ret == ProtoSSL::ProtoSslDtlsQueue::SEND_SUCCESS)
                    break;
                if (ret == ProtoSSL::ProtoSslDtlsQueue::CONN_SHUTDOWN)
                    return;
                usleep(1000);
            }
            usleep(2000); // 2ms pacing
        }
    }

    static void * _bulk_sender_thread(void * arg)
    {
        ((TestSession*)arg)->bulk_sender();
        return NULL;
    }

    void bulk_sender(void)
    {
        for (uint32_t seq = 0; seq < scen.num_bulk_msgs && !client_done && !session_failed; seq++)
        {
            ClientToServer msg;
            msg.set_type(CTS_STRESS_MSG);
            StressMsg * sm = msg.mutable_stress();
            sm->set_queue_id(queue_bulk);
            sm->set_seq(seq);
            sm->set_total_msgs(scen.num_bulk_msgs);
            sm->set_is_done(seq == scen.num_bulk_msgs - 1);
            std::string payload = make_payload(seq, 1024);
            sm->set_payload(payload);
            sm->set_crc(calc_crc32(payload));

            while (!client_done && !session_failed)
            {
                ProtoSSL::ProtoSslDtlsQueue::send_return_t ret =
                    client_dtlsq->send_message(queue_bulk, msg);
                if (ret == ProtoSSL::ProtoSslDtlsQueue::SEND_SUCCESS)
                    break;
                if (ret == ProtoSSL::ProtoSslDtlsQueue::CONN_SHUTDOWN)
                    return;
                usleep(500); // 0.5ms backoff on QOS_DROP
            }
        }
    }

    void failure(const std::string &reason)
    {
        WaitUtil::Lock l(&lock);
        if (!session_failed)
        {
            session_failed = true;
            failure_reason = reason;
            if (verbose)
                fprintf(stderr, "SESSION ERROR: %s\n", reason.c_str());
        }
    }
};

// =========================================================================
// Suite Registry & Main Runner
// =========================================================================

static std::vector<StressScenario> get_default_suite(void)
{
    std::vector<StressScenario> suite;

    // 1. Clean Baseline
    {
        StressScenario s;
        s.id = 1;
        s.name = "Clean Baseline";
        s.description = "0% loss, immediate delivery";
        s.fault.drop_percent = 0;
        s.fault.burst_drop_interval = 0;
        s.fault.burst_drop_count = 0;
        s.fault.delay_ms = 0;
        s.fault.duplicate_percent = 0;
        s.fragment_size = 64;
        s.window_size = 64;
        s.num_ctrl_msgs = 50;
        s.num_bulk_msgs = 50;
        s.max_missed_heartbeats = 3;
        s.expect_resends = false;
        suite.push_back(s);
    }

    // 2. Light Uniform Loss
    {
        StressScenario s;
        s.id = 2;
        s.name = "Light Loss (5%)";
        s.description = "5% uniform packet drop";
        s.fault.drop_percent = 5;
        s.fault.burst_drop_interval = 0;
        s.fault.burst_drop_count = 0;
        s.fault.delay_ms = 0;
        s.fault.duplicate_percent = 0;
        s.fragment_size = 64;
        s.window_size = 64;
        s.num_ctrl_msgs = 80;
        s.num_bulk_msgs = 80;
        s.max_missed_heartbeats = 3;
        s.expect_resends = true;
        suite.push_back(s);
    }

    // 3. Moderate Loss + Latency Jitter
    {
        StressScenario s;
        s.id = 3;
        s.name = "Loss (15%) + Delay (10ms)";
        s.description = "15% drop with 10ms latency";
        s.fault.drop_percent = 15;
        s.fault.burst_drop_interval = 0;
        s.fault.burst_drop_count = 0;
        s.fault.delay_ms = 10;
        s.fault.duplicate_percent = 0;
        s.fragment_size = 64;
        s.window_size = 64;
        s.num_ctrl_msgs = 80;
        s.num_bulk_msgs = 80;
        // increase missed heartbeat counter, to prevent LINK_DOWN messages
        s.max_missed_heartbeats = 6;
        s.expect_resends = true;
        suite.push_back(s);
    }

    // 4. Heavy Uniform Loss
    {
        StressScenario s;
        s.id = 4;
        s.name = "Heavy Loss (25%)";
        s.description = "25% aggressive loss recovery";
        s.fault.drop_percent = 25;
        s.fault.burst_drop_interval = 0;
        s.fault.burst_drop_count = 0;
        s.fault.delay_ms = 0;
        s.fault.duplicate_percent = 0;
        s.fragment_size = 64;
        s.window_size = 64;
        s.num_ctrl_msgs = 60;
        s.num_bulk_msgs = 60;
        // increase missed heartbeat counter, to prevent LINK_DOWN messages
        s.max_missed_heartbeats = 6;
        s.expect_resends = true;
        suite.push_back(s);
    }

    // 5. Periodic Burst Loss
    {
        StressScenario s;
        s.id = 5;
        s.name = "Burst Loss (4 of 20)";
        s.description = "Drops 4 consecutive packets every 20";
        s.fault.drop_percent = 0;
        s.fault.burst_drop_interval = 20;
        s.fault.burst_drop_count = 4;
        s.fault.delay_ms = 0;
        s.fault.duplicate_percent = 0;
        s.fragment_size = 64;
        s.window_size = 64;
        s.num_ctrl_msgs = 60;
        s.num_bulk_msgs = 60;
        s.max_missed_heartbeats = 3;
        s.expect_resends = true;
        suite.push_back(s);
    }

    // 6. Tiny Fragment Stress
    {
        StressScenario s;
        s.id = 6;
        s.name = "Tiny Frags (32B) + 10% Loss";
        s.description = "32-byte chunks (32+ frags/msg) with loss";
        s.fault.drop_percent = 10;
        s.fault.burst_drop_interval = 0;
        s.fault.burst_drop_count = 0;
        s.fault.delay_ms = 0;
        s.fault.duplicate_percent = 0;
        s.fragment_size = 32;
        s.window_size = 64;
        s.num_ctrl_msgs = 50;
        s.num_bulk_msgs = 50;
        s.max_missed_heartbeats = 3;
        s.expect_resends = true;
        suite.push_back(s);
    }

    // 7. Multi-Queue Backpressure
    {
        StressScenario s;
        s.id = 7;
        s.name = "Priority vs Bulk Backpressure";
        s.description = "100 bulk msgs flooding Queue 2 with 10% loss";
        s.fault.drop_percent = 10;
        s.fault.burst_drop_interval = 0;
        s.fault.burst_drop_count = 0;
        s.fault.delay_ms = 0;
        s.fault.duplicate_percent = 0;
        s.fragment_size = 64;
        s.window_size = 64;
        s.num_ctrl_msgs = 100;
        s.num_bulk_msgs = 100;
        s.max_missed_heartbeats = 3;
        s.expect_resends = true;
        suite.push_back(s);
    }

    return suite;
}

int main(int argc, char ** argv)
{
    int target_id = 0;
    bool verbose = false;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0)
        {
            verbose = true;
        }
        else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--list") == 0)
        {
            auto suite = get_default_suite();
            printf("Available Scenarios:\n");
            for (auto &s : suite)
                printf("  %d: %-30s - %s\n", s.id, s.name.c_str(), s.description.c_str());
            return 0;
        }
        else if (atoi(argv[i]) > 0)
        {
            target_id = atoi(argv[i]);
        }
    }

    auto full_suite = get_default_suite();
    std::vector<StressScenario> scenarios;
    if (target_id > 0)
    {
        for (auto &s : full_suite)
            if (s.id == target_id)
                scenarios.push_back(s);
        if (scenarios.empty())
        {
            fprintf(stderr, "Invalid scenario ID: %d\n", target_id);
            return 1;
        }
    }
    else
    {
        scenarios = full_suite;
    }

    printf("\n");
    printf("========================================================================================\n");
    printf("                  ProtoSslDtlsQueue Regression & Stress Test Suite                      \n");
    printf("========================================================================================\n");
    printf("Executing %zu test scenario(s)...\n\n", scenarios.size());

    short base_port = 19600;
    std::vector<std::pair<StressScenario, ScenarioResult>> results;
    int passed_count = 0;
    double total_time = 0.0;

    for (size_t i = 0; i < scenarios.size(); i++)
    {
        auto &scen = scenarios[i];
        printf("[%zu/%zu] Running: %-32s ... ", i + 1, scenarios.size(), scen.name.c_str());
        fflush(stdout);

        TestSession session(scen, base_port, verbose);
        base_port += 4; // increment for next scenario

        ScenarioResult res = session.run();
        results.push_back(std::make_pair(scen, res));
        total_time += res.elapsed_sec;

        if (res.passed)
        {
            passed_count++;
            printf("\033[32mPASS\033[0m (%.2fs, resents:%" PRIu64 ", drops:%u)\n",
                   res.elapsed_sec, res.frags_resent, res.packets_dropped);
        }
        else
        {
            printf("\033[31mFAIL\033[0m (%.2fs) - %s\n", res.elapsed_sec, res.error_msg.c_str());
        }
    }

    // Print Formatted Report Table
    printf("\n");
    printf("====================================================================================================\n");
    printf("                                       TEST SUMMARY REPORT                                          \n");
    printf("====================================================================================================\n");
    printf(" ID  Scenario Name                     Loss%% Burst FrgSz  Msgs  Resent Dropped    Bytes Status  Time\n");
    printf("----------------------------------------------------------------------------------------------------\n");

    for (auto &r : results)
    {
        auto &s = r.first;
        auto &res = r.second;

        char loss_str[16];
        if (s.fault.drop_percent > 0)
            snprintf(loss_str, sizeof(loss_str), "%2d%%", s.fault.drop_percent);
        else
            snprintf(loss_str, sizeof(loss_str), "  -");

        char burst_str[16];
        if (s.fault.burst_drop_count > 0)
            snprintf(burst_str, sizeof(burst_str), "%d/%d", s.fault.burst_drop_count, s.fault.burst_drop_interval);
        else if (s.fault.duplicate_percent > 0)
            snprintf(burst_str, sizeof(burst_str), "dup");
        else
            snprintf(burst_str, sizeof(burst_str), "  - ");

        const char *status_str = res.passed ? "\033[32mPASS\033[0m" : "\033[31mFAIL\033[0m";

        printf(" %2d  %-32s %5s %5s %5u %5u %7" PRIu64 " %7u %8" PRIu64 " %s  %4.2fs\n",
               s.id,
               s.name.c_str(),
               loss_str,
               burst_str,
               s.fragment_size,
               s.num_ctrl_msgs + s.num_bulk_msgs,
               res.frags_resent,
               res.packets_dropped,
               res.bytes_sent,
               status_str,
               res.elapsed_sec);
    }

    printf("====================================================================================================\n");
    printf(" Results: %d/%zu PASSED (%d FAILED)  |  Total Elapsed Time: %.2fs\n",
           passed_count, scenarios.size(), (int)scenarios.size() - passed_count, total_time);
    printf("====================================================================================================\n\n");

    return (passed_count == (int)scenarios.size()) ? 0 : 1;
}
