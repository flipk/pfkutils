
// todo:
//    -Ir : generate random data as input
//    -Iz : generate zero data as input

#include "libprotossl.h"
#include "i3_options.h"
#include I3_PROTO_HDR
#include <mbedtls/sha256.h>

using namespace ProtoSSL;
using namespace std;
using namespace PFK::i3;

#define I3_APP_NAME "PFK_i3"

class i3_program
{
    i3_options             opts;
    ProtoSSLCertParams  *  certs;
    ProtoSSLMsgs        *  msgs;
    bool                   connected;
    bool                   reading_input;
    ProtoSSLConnClient  *  client;
    ProtoSSLConnServer  *  server;
    i3Msg                  inMsg;
    i3Msg                  outMsg;
    pxfe_string            readbuffer;
    uint64_t               bytes_sent;
    uint64_t               bytes_received;
    mbedtls_sha256_context recv_hash;
    mbedtls_sha256_context send_hash;
    time_t                 last_stats;
    pxfe_timeval           tv_start;
    uint32_t               ping_seq;
    uint32_t               preload_count;
    pxfe_timeval           roundtrip;
    bool                   roundtrip_set;
public:
    i3_program(int argc, char ** argv)
        : opts(argc, argv)
    {
        certs = NULL;
        msgs = NULL;
        client = NULL;
        server = NULL;
        connected = false;
        reading_input = false;
        bytes_sent = 0;
        bytes_received = 0;
        mbedtls_sha256_init(&recv_hash);
        mbedtls_sha256_init(&send_hash);
        time(&last_stats);
        tv_start.getNow();
        ping_seq = 1;
        preload_count = opts.pingack_preload;
        roundtrip_set = false;
    }
    ~i3_program(void)
    {
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
        msgs = new ProtoSSLMsgs(opts.debug_flag > 1);

        certs = new ProtoSSLCertParams(opts.ca_cert_path,
                              opts.my_cert_path,
                              opts.my_key_path,
                              opts.my_key_password);

        if (msgs->loadCertificates(*certs) == false)
        {
            cerr << "certificate loading failed, try with -d\n";
            return 1;
        }

        if (opts.outbound)
        {
            client = msgs->startClient(opts.hostname, opts.port_number);
            if (client == NULL)
            {
                cerr << "failure to start client\n";
                return 1;
            }
            connected = true;
            send_proto_version();
            check_peer();
        }
        else
        {
            server = msgs->startServer(opts.port_number);
            if (server == NULL)
            {
                cerr << "failure to start server\n";
                return 1;
            }
        }

        mbedtls_sha256_starts(&recv_hash,0);
        mbedtls_sha256_starts(&send_hash,0);

        bool done = false;
        while (!done)
        {
            pxfe_select sel;

            if (server)
                sel.rfds.set(server->get_fd());
            if (client)
                sel.rfds.set(client->get_fd());
            if (connected && reading_input)
                sel.rfds.set(opts.input_fd);
            sel.tv.set(1,0);

            sel.select();

            if (server && sel.rfds.is_set(server->get_fd()))
            {
                ProtoSSLConnClient * newclient =
                    server->handle_accept();

                if (newclient)
                {
                    if (client)
                    {
                        cerr << "rejecting new connection\n";
                        delete newclient;
                    }
                    else
                    {
                        client = newclient;
                        connected = true;
                        send_proto_version();
                        check_peer();
                        tv_start.getNow();
                        if (opts.verbose)
                            print_stats(/*final*/ false);
                    }
                }
            }
            if (client && sel.rfds.is_set(client->get_fd()))
            {
                switch (client->handle_read(inMsg))
                {
                case ProtoSSLConnClient::GOT_DISCONNECT:
                    done = true;
                    break;
                case ProtoSSLConnClient::READ_MORE:
                    // nothing to do
                    break;
                case ProtoSSLConnClient::GOT_MESSAGE:
                    done = !handle_message();
                    break;
                }
            }
            if (connected && sel.rfds.is_set(opts.input_fd))
            {
                readbuffer.resize(16000);
                int cc = ::read(opts.input_fd,
                                readbuffer.vptr(), readbuffer.length());
                if (cc > 0)
                {
                    readbuffer.resize(cc);
                    send_file_msg();
                    bytes_sent += cc;
                }
                else
                {
                    send_file_done();
                    done = true;
                }
            }
            if (opts.verbose)
            {
                time_t now = time(NULL);
                if (now != last_stats)
                {
                    print_stats(/*final*/ false);
                    last_stats = now;
                }
            }
        }

        if (opts.verbose)
            print_stats(/*final*/ true);

        return 0;
    }
private:
    void check_peer(void)
    {
        ProtoSSLPeerInfo  info;
        if (client->get_peer_info(info) == false)
        {
            cerr << "ERROR: unable to fetch peer certificate info!\n";
            return;
        }
        if (opts.verbose || opts.debug_flag)
        {
            if (opts.verbose)
                cerr << endl;
            cerr << "connection: " << info.ipaddr
                 << " " << info.common_name
                 << " " << info.pkcs9_email
                 << " " << info.org_unit
                 << endl;
        }
    }
    void sendmsg(void)
    {
        if (opts.debug_flag)
            fprintf(stderr, "sending message: %s\n",
                    outMsg.DebugString().c_str());
        client->send_message(outMsg);
        outMsg.Clear();
    }
    void send_proto_version(void)
    {
        outMsg.set_type(i3_VERSION);
        outMsg.mutable_proto_version()->set_app_name(I3_APP_NAME);
        outMsg.mutable_proto_version()->set_version(i3_VERSION_1);
        sendmsg();
    }
    // return false for failure
    bool handle_message(void)
    {
        if (opts.debug_flag)
            fprintf(stderr, "received message: %s\n",
                    inMsg.DebugString().c_str());
        switch (inMsg.type())
        {
        case i3_VERSION:
            if (inMsg.has_proto_version() == false)
            {
                cerr << "malformed VERSION message from peer\n";
                return false;
            }
            if (inMsg.proto_version().version() != i3_VERSION_1)
            {
                cerr << "remote app " << inMsg.proto_version().app_name()
                     << " reported incorrect version "
                     << inMsg.proto_version().version()
                     << " (should be " << i3_VERSION_1 << ")\n";
                return false;
            }
            if (opts.input_set)
                reading_input = true;
            break;
        case i3_FILEDATA:
            if (inMsg.has_file_data() == false)
            {
                cerr << "malformed FILEDATA message from peer\n";
                return false;
            }
            if (opts.output_set)
            {
                const pxfe_string &data =
                    static_cast<const pxfe_string &>(
                        inMsg.file_data().file_data());
                int len = data.length();
                int cc = ::write(opts.output_fd, data.vptr(), len);
                if (cc != len)
                {
                    cerr << "failure to write (" << cc << " != "
                         << len << ")\n";
                    return false;
                }
                mbedtls_sha256_update(&recv_hash, data.ucptr(), data.length());
                bytes_received += cc;
            }
            if (inMsg.file_data().has_ping())
            {
                outMsg.set_type(i3_PINGACK);
                Ping * p = outMsg.mutable_ping_ack();
                p->CopyFrom(inMsg.file_data().ping());
                sendmsg();
            }
            break;
        case i3_PINGACK:
            handle_pingack();
            break;
        case i3_DONE:
            handle_file_done();
            break;
        }
        return true;
    }
    void send_file_msg(void)
    {
        outMsg.set_type(i3_FILEDATA);
        FileData * fd = outMsg.mutable_file_data();
        fd->set_file_data(readbuffer);
        if (opts.pingack)
        {
            preload_count--;
            if (preload_count <= 0)
            {
                Ping * p = fd->mutable_ping();
                p->set_seq(ping_seq++);
                pxfe_timeval tv;
                tv.getNow();
                p->set_time_sec(tv.tv_sec);
                p->set_time_usec(tv.tv_usec);
                reading_input = false;
                preload_count = opts.pingack_preload;
            }
        }
        sendmsg();
        mbedtls_sha256_update(&send_hash,
                              readbuffer.ucptr(),
                              readbuffer.length());
    }
    void send_file_done(void)
    {
        pxfe_string  sent_hash;
        sent_hash.resize(32);
        mbedtls_sha256_finish(&send_hash, sent_hash.ucptr());
        outMsg.set_type(i3_DONE);
        FileDone * fd = outMsg.mutable_file_done();
        fd->set_file_size(bytes_sent);
        fd->set_sha256(sent_hash);
        sendmsg();
    }
    void handle_pingack(void)
    {
        pxfe_timeval ts, now;
        now.getNow();
        const Ping &p = inMsg.ping_ack();
        ts.set(p.time_sec(), p.time_usec());
        roundtrip = now - ts;
        roundtrip_set = true;
        reading_input = opts.input_set;
        if (opts.debug_flag)
            fprintf(stderr, "round trip calculated : %u.%06u\n",
                   (unsigned int) roundtrip.tv_sec,
                   (unsigned int) roundtrip.tv_usec);
    }
    void handle_file_done(void)
    {
        const FileDone &fd = inMsg.file_done();
        if (bytes_received != fd.file_size())
            cerr << "FILE SIZE mismatch! ERROR in transfer:\n"
                 << "calculated size " << bytes_received << "\n"
                 << "received size   " << fd.file_size() << "\n";
        pxfe_string rcvd_hash;
        rcvd_hash.resize(32);
        mbedtls_sha256_finish(&recv_hash, rcvd_hash.ucptr());
        std::string one = rcvd_hash.format_hex();
        std::string two = ((pxfe_string&) fd.sha256()).format_hex();
        if (one != two)
            cerr << "SHA256 sum mismatch! ERROR in transfer:\n"
                 << "calculated hash '" << one << "'\n"
                 << "received hash   '" << two << "'\n";
    }
    void print_stats(bool final)
    {
        pxfe_timeval now, diff;
        uint64_t total = bytes_sent + bytes_received;
        now.getNow();
        diff = now - tv_start;
        float t = diff.usecs() / 1000000.0;
        if (t == 0.0)
            t = 99999.0;
        float bytes_per_sec = (float) total / t;
        fprintf(stderr, "\r%" PRIu64 " in %u.%06u s "
               "(%.0f bps)",
               total,
               (unsigned int) diff.tv_sec,
               (unsigned int) diff.tv_usec,
               bytes_per_sec);
        if (roundtrip_set)
            fprintf(stderr, " (rt %u.%06u)",
                   (unsigned int) roundtrip.tv_sec,
                   (unsigned int) roundtrip.tv_usec);
        if (final)
            fprintf(stderr, "\n");
    }
};

extern "C" int
i3_main(int argc, char ** argv)
{
    i3_program  i3(argc, argv);
    if (i3.get_ok() == false)
        return 1;
    return i3.main();
}
