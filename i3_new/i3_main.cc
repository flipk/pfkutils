
#include "libprotossl2.h"
#include "i3_options.h"
#include I3_PROTO_HDR

using namespace ProtoSSL;
using namespace std;
using namespace PFK::i3;

#define I3_APP_NAME "PFK_i3"

class i3_program
{
    i3_options           opts;
    ProtoSSLCertParams * certs;
    ProtoSSLMsgs       * msgs;
    bool                 connected;
    bool                 reading_input;
    ProtoSSLConnClient * client;
    ProtoSSLConnServer * server;
    i3Msg                inMsg;
    i3Msg                outMsg;
    pxfe_string          readbuffer;
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
            if (connected && reading_input && sel.rfds.is_set(opts.input_fd))
            {
                readbuffer.resize(16000);
                int cc = ::read(opts.input_fd,
                                readbuffer.vptr(), readbuffer.length());
                if (cc > 0)
                {
                    readbuffer.resize(cc);
                    send_file_msg();
                }
                else
                {
                    done = true;
                }
            }
        }

        return 0;
    }
private:
    void sendmsg(void)
    {
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
            }
            break;
        case i3_DONE:
            break;
        }
        return true;
    }
    void send_file_msg(void)
    {
        outMsg.set_type(i3_FILEDATA);
        outMsg.mutable_file_data()->set_file_data(readbuffer);
        sendmsg();
    }
};

int
main(int argc, char ** argv)
{
    i3_program  i3(argc, argv);
    if (i3.get_ok() == false)
        return 1;
    return i3.main();
}
