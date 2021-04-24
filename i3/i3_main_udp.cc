
#define __STDC_FORMAT_MACROS 1

#include "pfkutils_config.h"
#include "libprotossl.h"
#include "i3_options.h"
#include I3_PROTO_HDR
#include <mbedtls/sha256.h>
#include <pthread.h>
#include "posix_fe.h"

using namespace ProtoSSL;
using namespace std;
using namespace PFK::i3;

#define I3_APP_NAME "PFK_i3"

class i3_udp_program
{
    i3_options              opts;
    ProtoSSLCertParams  *   certs;
    ProtoSSLMsgs        *   msgs;
    ProtoSSLConnServer  *   server;
    ProtoSSLConnClient  *   client;
    ProtoSslDtlsQueue   *   dtlsq;
    uint32_t                dtlsq_ind;
    ProtoSslDtlsQueueConfig dtlsq_config;
    mbedtls_sha256_context  recv_hash;
    mbedtls_sha256_context  send_hash;
    bool                    connected;
    bool                    client_running;
    bool                    localreader_running;
    bool                    stats_thread_running;
    pthread_t               server_thread_id;
    pthread_t               client_thread_id;
    pthread_t               stats_thread_id;
    pthread_t               localreader_thread_id;
    pxfe_pipe               localreader_closerpipe;
    pxfe_pthread_mutex      lock;
    bool                    localreader_waiting;
    void      (i3_udp_program::*threadfunc)(void);
    pxfe_pthread_mutex      thread_start_sync;
    i3Msg                   inMsg;
    i3Msg                   outMsg;
public:
    i3_udp_program(int argc, char ** argv)
        : opts(argc, argv)
    {
        certs = NULL;
        msgs = NULL;
        server = NULL;
        client = NULL;
        dtlsq = NULL;
        client_running = false;
        localreader_running = false;
        stats_thread_running = false;
        localreader_waiting = false;
        mbedtls_sha256_init(&recv_hash);
        mbedtls_sha256_init(&send_hash);
        connected = false;
//        dtlsq_config.fragment_size = ?;
//        dtlsq_config.ticks_per_second = ?;
        dtlsq_config.hearbeat_interval =
            dtlsq_config.ticks_per_second * 10;
        dtlsq_config.max_missed_heartbeats = 6; // one minute.
        dtlsq_config.add_queue(
            dtlsq_ind,
            true, // reliable
            ProtoSslDtlsQueueConfig::FIFO, // queue type
            ProtoSslDtlsQueueConfig::BYTES, // limit type
            1000000 // limit
            );
        thread_start_sync.attr.settype(PTHREAD_MUTEX_NORMAL);
        thread_start_sync.init();
        lock.init();
        lock.lock(); // place in locked state permanently.
    }
    ~i3_udp_program(void)
    {
        if (dtlsq)
            delete dtlsq;
        if (server)
            delete server;
        if (client)
            delete client;
        if (msgs)
            delete msgs;
        if (certs)
            delete certs;
        mbedtls_sha256_free(&recv_hash);
        mbedtls_sha256_free(&send_hash);
    }
    bool get_ok(void) const { return opts.ok; }
    int main(void)
    {
        msgs = new ProtoSSLMsgs(false, // blocking
                                opts.debug_flag > 1,
                                0, // read_timeout
                                false, // use_tcp
                                1000, // dtls_timeout_min
                                2000  // dtls_timeout_max
            );

        certs = new ProtoSSLCertParams(opts.ca_cert_path,
                              opts.my_cert_path,
                              opts.my_key_path,
                              opts.my_key_password);

        if (msgs->loadCertificates(*certs) == false)
        {
            cerr << "certificate loading failed, try with -d\n";
            return 1;
        }

        if (msgs->validateCertificates() == false)
        {
            cerr << "certificates did not validate!\n";
            return 1;
        }

        MBEDTLS_SHA256_STARTS(&recv_hash,0);
        MBEDTLS_SHA256_STARTS(&send_hash,0);

        void *thread_ret = NULL;
        if (opts.outbound)
        {
            // in outbound mode, we're forming a client object immediately
            // connecting out to the destination IP that was specified,
            // and going straight to connected state.
            client = msgs->startClient(opts.hostname, opts.port_number);
            if (client == NULL)
            {
                cerr << "failure to start client\n";
                return 1;
            }
            connected = true;

            dtlsq = new ProtoSSL::ProtoSslDtlsQueue(
                dtlsq_config, client);

            // DtlsQ owns the client now.
            client = NULL;

            // this function doesn't return until client thread
            // is valid and running.
            threadstarter(&client_thread_id, &i3_udp_program::client_thread);
        }
        else
        {
            // in inbound mode, we're creating a listening tcp socket
            // (server) and waiting for a new connection to appear.
            // thus we are not in a connected state right away.
            server = msgs->startServer(opts.port_number);
            if (server == NULL)
            {
                cerr << "failure to start server\n";
                return 1;
            }

            // this function doesn't return until server thread
            // is valid and running.
            threadstarter(&server_thread_id, &i3_udp_program::server_thread);

            // server thread completes when it has started
            // a client thread.
            pthread_join(server_thread_id,  &thread_ret);
        }

        // wait for client threads to finish.
        if (client_running)
            pthread_join(client_thread_id, &thread_ret);
        if (localreader_running)
            pthread_join(localreader_thread_id, &thread_ret);
        if (stats_thread_running)
            pthread_join(stats_thread_id, &thread_ret);

        if (dtlsq)
            dtlsq->shutdown();

        return 0;
    }
private:

    void threadstarter(pthread_t *id,
                       void (i3_udp_program::*func)(void))
    {
        pxfe_pthread_attr  attr;
        attr.set_detach(false);
        // lock once.
        pxfe_pthread_mutex_lock  lock(thread_start_sync);
        threadfunc = func;
        pthread_create(id, attr(), &_threadstarter, (void*) this);
        // lock again; blocks here until thread unlocks.
        thread_start_sync.lock();
    }
    static void *_threadstarter(void *arg)
    {
        i3_udp_program *i3p = (i3_udp_program *) arg;
        void (i3_udp_program::*func)(void) = i3p->threadfunc;
        i3p->thread_start_sync.unlock(); // signal
        (i3p->*func)();
        return NULL;
    }

    void server_thread(void)
    {
        bool done = false;
        while (!done)
        {
            client = server->handle_accept();
            if (client)
            {
                connected = true;
                dtlsq = new ProtoSSL::ProtoSslDtlsQueue(
                    dtlsq_config, client);
                if (dtlsq)
                {
                    client = NULL;
                    if (dtlsq->ok())
                    {
                        // dtlsq owns the client now.
                        threadstarter(&client_thread_id,
                                      &i3_udp_program::client_thread);
                        // dont need server anymore
                        delete server;
                        server = NULL;
                        done = true; // only one connection accepted.
                    }
                    else
                    {
                        delete dtlsq;
                        dtlsq = NULL;
                    }
                }
                else
                {
                    delete client;
                    client = NULL;
                }
            }
        }
    }

    void stats_thread(void)
    {
        ProtoSslDtlsQueueStatistics  stats;
        stats_thread_running = true;
        bool done = false;
        uint64_t last_bytes = 0;
        while (!done)
        {
            pxfe_select  sel;
            sel.rfds.set(localreader_closerpipe.readEnd);
            sel.tv.set(1,0);
            sel.select();
            if (sel.rfds.is_set(localreader_closerpipe.readEnd))
                done = true;

            if (dtlsq)
                dtlsq->get_stats(&stats);

            uint64_t bytes = stats.bytes_received - last_bytes;
            last_bytes = stats.bytes_received;

            fprintf(stderr, "%s brps %u\n",
                    stats.Format().c_str(), (uint32_t) bytes);
        }
    }

    void client_thread(void)
    {
        client_running = true;
        check_peer();
        send_proto_version();

        threadstarter(&stats_thread_id, &i3_udp_program::stats_thread);

        bool done = false;
        while (!done)
        {
            ProtoSSL::ProtoSslDtlsQueue::read_return_t ret;
            ret = dtlsq->handle_read(inMsg);
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
                done = handle_message();
                break;

            case ProtoSSL::ProtoSslDtlsQueue::LINK_DOWN:
                fprintf(stderr, "dtlsq LINK-DOWN!\n");
                break;

            case ProtoSSL::ProtoSslDtlsQueue::LINK_UP:
                fprintf(stderr, "dtlsq LINK-UP!\n");
                break;
            }
        }
        connected = false;
        localreader_closerpipe.write((void*)" ", 1);
    }

    // return true if done
    bool handle_message(void)
    {
        bool done = false;

        switch (inMsg.type())
        {
        case i3_VERSION:
            if (localreader_running == false)
                threadstarter(&localreader_thread_id,
                              &i3_udp_program::localreader_thread);
            break;

        case i3_FILEDATA:
            if (inMsg.has_file_data())
            {
                ssize_t towrite = inMsg.file_data().file_data().length();
//                fprintf(stderr,"about to write %d\n", (int) towrite);
                if (towrite > 0)
                {
                    void * ptr = (void*)
                        inMsg.file_data().file_data().c_str();
                    ssize_t s = ::write(1, ptr, towrite);
                    if (s != towrite)
                    {
                        fprintf(stderr, "::write failed: %d (%s)\n",
                                errno, strerror(errno));
                    }
                }
                if (inMsg.file_data().has_ping())
                {
                    const Ping &p = inMsg.file_data().ping();
                    send_pingack(p.seq(), p.time_sec(), p.time_usec());
                }
            }
            break;

        case i3_DONE:
            done = true;
            break;

        case i3_PINGACK:
        {
            if (inMsg.has_ping_ack())
            {
                const Ping &p = inMsg.ping_ack();
                pxfe_timeval  tvnow;
                tvnow.getNow();
                pxfe_timeval  tv((time_t) p.time_sec(),
                                 (long)   p.time_usec());
                pxfe_timeval diff = tvnow - tv;

//                printf("calculated RTD %" PRIu64 " usecs\n",
//                       diff.usecs());

                if (localreader_waiting)
                    lock.unlock(); // release suspended reader
            }

            break;
        }
        }

        return done;
    }

    void localreader_thread(void)
    {
        uint32_t outseq = 1;
        int seqcount = 0;
        localreader_running = true;
        bool done = false;
        while (!done)
        {
            pxfe_select  sel;
            sel.rfds.set(0);
            sel.rfds.set(localreader_closerpipe.readEnd);
            sel.tv.set(1,0);
            sel.select();
            if (sel.rfds.is_set(localreader_closerpipe.readEnd))
                break;
            if (sel.rfds.is_set(0))
            {
                bool block = false;

                outMsg.set_type(i3_FILEDATA);
                std::string *data =
                    outMsg.mutable_file_data()->mutable_file_data();
                data->resize(8000);
                ssize_t s = ::read(0, (void*) data->c_str(), 4000);
                if (s > 0)
                {
                    data->resize(s);
                    if (++seqcount >= 100)
                    {
                        seqcount = 0;
                        Ping *pingmsg =
                            outMsg.mutable_file_data()->mutable_ping();
                        pingmsg->set_seq(outseq++);
                        pxfe_timeval tv;
                        tv.getNow();
                        pingmsg->set_time_sec ((uint32_t) tv.tv_sec);
                        pingmsg->set_time_usec((uint32_t) tv.tv_usec);
                        block = true;
                    }
                    sendmsg();
                    if (block)
                    {
                        localreader_waiting = true;
                        lock.lock(); // block here.
                    }
                }
                else
                {
                    outMsg.Clear();
                    done = true;
                }
            }
        }
        send_done();
    }

    void check_peer(void)
    {
#if 0 // xxx broken -- we don't use 'client' pointer here.
        ProtoSSLPeerInfo  info;
        if (client->get_peer_info(info) == false)
        {
            // we always print this information regardless of verbose
            // or debug settings, because this might represent a peer
            // trying to fool us into connecting, and the user should
            // be told that.
            cerr << "ERROR: unable to fetch peer certificate info!\n";
            return;
        }
        if (opts.verbose || opts.debug_flag)
        {
            // we always collect this info but don't display it
            // unless the user has indicated they want verbose or debug.
            if (opts.verbose)
                cerr << endl;
            cerr << "connection: " << info.ipaddr
                 << " " << info.common_name
                 << " " << info.pkcs9_email
                 << " " << info.org_unit
                 << endl;
        }
#endif
    }
    void sendmsg(void)
    {
        // a wrapper for client->send_message because you want debug
        // prints every place that sends messages, and you want to never
        // forget to Clear the message (so the optional contents of this
        // message don't leak into the next message and so on).
        if (opts.debug_flag)
            fprintf(stderr, "sending message: %s\n",
                    outMsg.DebugString().c_str());
        ProtoSslDtlsQueue::send_return_t sendret =
            dtlsq->send_message(dtlsq_ind, outMsg);
        switch (sendret)
        {
        case ProtoSslDtlsQueue::SEND_SUCCESS:
            break;
        case ProtoSslDtlsQueue::MESSAGE_TOO_BIG:
            fprintf(stderr, "MESSAGE TOO BIG!\n");
            break;
        case ProtoSslDtlsQueue::BOGUS_QUEUE_NUMBER:
            fprintf(stderr, "BOGUS_QUEUE_NUMBER!\n");
            break;
        case ProtoSslDtlsQueue::MSG_NOT_INITIALIZED:
            fprintf(stderr, "MSG_NOT_INITIALIZED\n");
            break;
        case ProtoSslDtlsQueue::QOS_DROP:
            fprintf(stderr, "QOS_DROP\n");
            break;
        case ProtoSslDtlsQueue::CONN_SHUTDOWN:
            fprintf(stderr, "CONN_SHUTDOWN\n");
            break;
        }
        outMsg.Clear();
    }
    void send_pingack(uint32_t seq, uint32_t s, uint32_t us)
    {
        outMsg.set_type(i3_PINGACK);
        outMsg.mutable_ping_ack()->set_seq(seq);
        outMsg.mutable_ping_ack()->set_time_sec(s);
        outMsg.mutable_ping_ack()->set_time_usec(us);
        sendmsg();
    }
    void send_proto_version(void)
    {
        outMsg.set_type(i3_VERSION);
        outMsg.mutable_proto_version()->set_app_name(I3_APP_NAME);
        outMsg.mutable_proto_version()->set_version(i3_VERSION_1);
        sendmsg();
    }
    void send_done(void)
    {
        outMsg.set_type(i3_DONE);
        outMsg.mutable_file_done()->set_file_size(1); // xxxxxx
        // xxxxx outMsg.mutable_file_done()->set_sha256();
        sendmsg();
    }
};

extern "C" int
i3_udp_main(int argc, char ** argv)
{
    i3_udp_program  i3(argc, argv);
    if (i3.get_ok() == false)
        return 1;
    return i3.main();
}
