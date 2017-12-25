
#include "i3_protossl_conn.h"

i3protoConn :: i3protoConn(const i3_options &_opts,
                           i3_loop &_loop)
    : opts(_opts), loop(_loop)
{
    if (opts.debug_flag > 0)
        printf("i3protoConn :: i3protoConn\n");
}

i3protoConn :: ~i3protoConn(void)
{
    if (opts.debug_flag > 0)
        printf("i3protoConn :: ~i3protoConn\n");
    msgs->stop();
}

bool
i3protoConn :: messageHandler(const PFK::i3::i3Msg &msg)
{
    i3_evt * evt;
    PFK::i3::i3Msg *newmsg;

    if (opts.debug_flag > 0)
        printf("****** i3protoConn :: messageHandler ******\n%s",
               msg.DebugString().c_str());

    switch (msg.type())
    {
    case PFK::i3::i3_VERSION:
        if (msg.has_proto_version() == false)
        {
            printf("malformed version message\n");
            return false;
        }
        if (opts.debug_flag > 0)
            printf("connection from remote app %s proto version %d\n",
                   msg.proto_version().app_name().c_str(),
                   msg.proto_version().version());
        if (msg.proto_version().version() != PFK::i3::PROTOCOL_VERSION_1)
        {
            printf("version mismatch (%d != %d)\n",
                   msg.proto_version().version(),
                   PFK::i3::PROTOCOL_VERSION_1);
            return false;
        }
        evt = loop.alloc_evt();
        evt->set_connect(this);
        loop.enqueue_evt(evt);
        break;

    case PFK::i3::i3_FILEDATA:
    case PFK::i3::i3_DONE:
        evt = loop.alloc_evt();
        newmsg = new PFK::i3::i3Msg;
        newmsg->CopyFrom(msg);
        evt->set_rcvmsg(newmsg);
        loop.enqueue_evt(evt);
        break;

    default:
        printf("i3protoConn :: messageHandler : unknown message!\n");
        return false;
    }

    return true;
}

void
i3protoConn :: handleConnect(void)
{
    if (opts.debug_flag > 0)
        printf("i3protoConn :: handleConnect\n");

    outMessage().set_type(PFK::i3::i3_VERSION);
    outMessage().mutable_proto_version()->set_app_name("PFK_i3");
    outMessage().mutable_proto_version()->set_version(
        PFK::i3::PROTOCOL_VERSION_1);
    sendMessage();
}

void
i3protoConn :: handleDisconnect(void)
{
    if (opts.debug_flag > 0)
        printf("i3protoConn :: handleDisconnect\n");
    i3_evt * evt = loop.alloc_evt();
    evt->set_disconnect(this);
    loop.enqueue_evt(evt);
}

void
i3protoConn :: send_read_data(const std::string &data)
{
    outMessage().set_type(PFK::i3::i3_FILEDATA);
    PFK::i3::FileData * fd = outMessage().mutable_file_data();
    fd->set_file_data(data);
    sendMessage();
}

void
i3protoConn :: send_read_done(uint64_t file_size,
                              const std::string &sha256_hash)
{
    outMessage().set_type(PFK::i3::i3_DONE);
    PFK::i3::FileDone * fd = outMessage().mutable_file_done();
    fd->set_file_size(file_size);
    fd->set_sha256(sha256_hash);
    sendMessage();
}
