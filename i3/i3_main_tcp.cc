
#include "pfkutils_config.h"
#include "libprotossl.h"
#include "i3_options.h"
#include I3_PROTO_HDR
#include <mbedtls/sha256.h>

using namespace ProtoSSL;
using namespace std;
using namespace PFK::i3;

#define I3_APP_NAME "PFK_i3"

class i3_tcp_program
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
    i3_tcp_program(int argc, char ** argv)
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
    ~i3_tcp_program(void)
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
            send_proto_version();
            check_peer();
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
        }

        MBEDTLS_SHA256_STARTS(&recv_hash,0);
        MBEDTLS_SHA256_STARTS(&send_hash,0);

        pxfe_ticker tick;
        tick.start(0,250000);
        bool done = false;
        while (!done)
        {
            pxfe_select sel;

            sel.rfds.set(tick.fd());
            if (server)
                sel.rfds.set(server->get_fd());
            if (client)
                sel.rfds.set(client->get_fd());
            // do not select for read on our local input
            // if we're no connected or if we sent a ping and
            // are waiting for a ping-ack to restart us.
            if (connected && reading_input)
                sel.rfds.set(opts.input_fd);
            // a timeout of 1s guarantees the stats get
            // printed on a regular basis, even if the transfer
            // goes completely idle (or stalled due to network issue).
            sel.tv.set(1,0);

            sel.select();

            if (server && sel.rfds.is_set(server->get_fd()))
            {
                ProtoSSLConnClient * newclient =
                    server->handle_accept();

                // handle_accept may return null for several reasons,
                // for instance if the certificate valiation failed.
                if (newclient)
                {
                    if (client)
                    {
                        // we only handle one client. if a new one
                        // comes in we should drop it. actually,
                        // we should delete the server object when
                        // we get a connection (closing the listening
                        // TCP socket) but that is not tested in
                        // libprotossl.
                        cerr << "rejecting new connection\n";
                        delete newclient;
                    }
                    else
                    {
                        client = newclient;
                        connected = true;
                        send_proto_version();
                        check_peer();
                        // reset the start time to "now" because
                        // we just accepted a connection and the
                        // time waiting for connection shouldn't
                        // count in the stats.
                        tv_start.getNow();
                        if (opts.verbose)
                            // print the first stats immediately.
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
                // the max size of an SSL frame is 16384, and the
                // protobuf has some overhead. so max read size is
                // enough to guarantee an i3Msg never exceeds 16384.
                int cc = readbuffer.read(opts.input_fd, 16000);
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
            if (sel.rfds.is_set(tick.fd()))
            {
                tick.doread();
                if (opts.verbose)
                {
                    print_stats(/*final*/ false);
                }
            }
        }
        tick.pause();

        // print one more stats before we exit with the latest
        // information so the last stats printed represents the full
        // file size transferred in the proper total time.
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
                // receipt of a valid version message is what tells
                // us it's okay to start reading from our local data
                // source and sending data to the other side.
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
                // cast the string in the protobuf to a pxfe_string
                // so we get those nice vptr and ucptr casts.
                const pxfe_string &data =
                    static_cast<const pxfe_string &>(
                        inMsg.file_data().file_data());
                int len = data.length();
                int cc = data.write(opts.output_fd);
                if (cc != len)
                {
                    cerr << "failure to write (" << cc << " != "
                         << len << ")\n";
                    return false;
                }
                MBEDTLS_SHA256_UPDATE(&recv_hash, data.ucptr(), len);
                bytes_received += cc;
            }
            if (inMsg.file_data().has_ping())
            {
                // if the sender piggybacked a ping request in this
                // data, it means they're stopping their input and waiting
                // for us to ping-ack before they'll start again.
                // echo back the timestamp so they can also calculate
                // round trip delay.
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
            // every preload_count packets, piggyback a ping
            // and stop. we assume a ping-ack will come back
            // as a result.
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
        MBEDTLS_SHA256_UPDATE(&send_hash,
                              readbuffer.ucptr(),
                              readbuffer.length());
    }
    void send_file_done(void)
    {
        // send our sha256 hash to the peer so he can
        // validate that what he wrote to his output file
        // matches what we read from ours.
        pxfe_string  sent_hash;
        sent_hash.resize(32);
        MBEDTLS_SHA256_FINISH(&send_hash, sent_hash.ucptr());
        outMsg.set_type(i3_DONE);
        FileDone * fd = outMsg.mutable_file_done();
        fd->set_file_size(bytes_sent);
        fd->set_sha256(sent_hash);
        sendmsg();
    }
    void handle_pingack(void)
    {
        // when we sent a ping, we included our own clock
        // at the time of the ping. the peer echoed that value back.
        // if we read our local clock and subtract that value,
        // we can calculate round trip delay.
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
        // peer says they're done sending data. they've sent their
        // sha256.  compare it to ours to make sure we've got the same
        // data.
        const FileDone &fd = inMsg.file_done();
        if (bytes_received != fd.file_size())
            cerr << "FILE SIZE mismatch! ERROR in transfer:\n"
                 << "calculated size " << bytes_received << "\n"
                 << "received size   " << fd.file_size() << "\n";
        pxfe_string rcvd_hash;
        rcvd_hash.resize(32);
        MBEDTLS_SHA256_FINISH(&recv_hash, rcvd_hash.ucptr());
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
               "(%.0f Bps %.0f bps)",
               total,
               (unsigned int) diff.tv_sec,
               (unsigned int) diff.tv_usec,
                bytes_per_sec, bytes_per_sec * 8.0);
        if (roundtrip_set)
            fprintf(stderr, " (rt %u.%06u)",
                   (unsigned int) roundtrip.tv_sec,
                   (unsigned int) roundtrip.tv_usec);
        if (final)
            fprintf(stderr, "\n");
    }
};

extern "C" int
i3_tcp_main(int argc, char ** argv)
{
    i3_tcp_program  i3(argc, argv);
    if (i3.get_ok() == false)
        return 1;
    return i3.main();
}
