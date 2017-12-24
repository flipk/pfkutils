
#include "i3_loop.h"
#include "i3_reader.h"
#include "i3_protossl_conn.h"

//static
const std::string i3_evt::evt_type_names[i3_evt::NUM_EVTS] = {
#define I3_EVT_TYPE(x) #x ,
        I3_EVT_TYPE_LIST
#undef  I3_EVT_TYPE
};

void i3_evt::init(type_e _type)
{
    type = _type;
    read_buffer.resize(0);
    msg = NULL;
    conn = NULL;
}

void i3_evt::set_connect(i3protoConn * _conn)
{
    init(CONNECT);
    conn = _conn;
}

void i3_evt::set_disconnect(i3protoConn * _conn)
{
    init(DISCONNECT);
    conn = _conn;
}

void i3_evt::set_read(std::string &buffer)
{
    init(READ);
    std::swap(buffer, read_buffer);
}

void i3_evt::set_readdone(void)
{
    init(READ_DONE);
}

void i3_evt::set_rcvmsg(PFK::i3::i3Msg *_msg)
{
    init(RCVMSG);
    msg = _msg;
}

void i3_evt::set_die(void)
{
    init(DIE);
}

i3_loop::i3_loop(const i3_options &_opts)
    : opts(_opts), conn(NULL)
{
    p.add(10);
}

//virtual
i3_loop::~i3_loop(void)
{
    stopjoin();
}

void
i3_loop::start(void)
{
    create();
}

/*virtual*/ void*
i3_loop::entry(void *arg)
{
    bool done = false;
    while (!done)
    {
        i3_evt * evt = q.dequeue(-1);
        if (evt == NULL)
            continue;
        if (opts.debug_flag)
            printf("i3_loop: got event of type %d (%s)\n",
                   evt->type, evt->type_name().c_str());
        switch (evt->type)
        {
        case i3_evt::CONNECT:
            reader->start();
            conn = evt->conn;
            break;
        case i3_evt::DISCONNECT:
            if (conn == evt->conn)
                conn = NULL;
            done = true;
            break;
        case i3_evt::READ:
            if (conn)
                conn->send_read_data(evt->read_buffer);
            break;
        case i3_evt::READ_DONE:
            if (conn)
                conn->send_read_done();
            break;
        case i3_evt::RCVMSG:
            done = handle_rcvmsg(evt->msg);
            delete evt->msg;
            break;
        case i3_evt::DIE:
            done = true;
            break;
        }
        p.release(evt);
    }
}

bool
i3_loop::handle_rcvmsg(const PFK::i3::i3Msg *msg)
{
    switch (msg->type())
    {
    case PFK::i3::i3_FILEDATA:
        if (msg->has_file_data() == false)
        {
            printf("ERROR: i3_loop::handle_rcvmsg no file_data 1\n");
            return true;
        }
        if (msg->file_data().has_file_data() == false)
        {
            printf("ERROR: i3_loop::handle_rcvmsg no file_data 2\n");
            return true;
        }
        if (opts.output_set)
        {
            int cc = ::write(opts.output_fd,
                             msg->file_data().file_data().c_str(),
                             msg->file_data().file_data().length());
        }
        break;

    case PFK::i3::i3_DONE:
        if (msg->has_file_done() == false)
        {
            printf("ERROR: i3_loop::handle_rcvmsg no file_done 1\n");
        }
        if (conn)
            conn->closeConnection();
        return true;

    default:
        printf("ERROR: i3_loop: unhandled proto msg type %d\n",
               msg->type());
        return true;
    }
    return false;
}

/*virtual*/ void
i3_loop::send_stop(void)
{
    if (running())
    {
        i3_evt * evt = p.alloc();
        evt->set_die();
        q.enqueue(evt);
    }
}
