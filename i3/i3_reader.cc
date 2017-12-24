
#include "i3_reader.h"

i3_reader::i3_reader(const i3_options &_opts, i3_loop &_loop)
    : opts(_opts), loop(_loop)
{
}

//virtual
i3_reader::~i3_reader(void)
{
    stopjoin();
}

void
i3_reader::start(void)
{
    create();
}

/*virtual*/ void *
i3_reader::entry(void *arg)
{
    std::string  buffer;
    // this max size is chosen to ensure the total
    // size of an i3Msg is less than the SSL maximum (16384).
    const int max_read = 16000;
    int cc = 0;
    bool done = false;

    while (!done)
    {
        pxfe_select sel;
        sel.rfds.set(opts.input_fd);
        sel.rfds.set(exitPipe.readEnd);
        sel.tv.set(1,0);

        sel.select();
        if (sel.rfds.is_set(opts.input_fd))
        {
            buffer.resize(max_read);
            cc = ::read(opts.input_fd, (void*) buffer.c_str(), max_read);
            if (cc <= 0)
                break;
            buffer.resize(cc);
            i3_evt * evt = loop.alloc_evt();
            evt->set_read(buffer);
            loop.enqueue_evt(evt);
        }
        if (sel.rfds.is_set(exitPipe.readEnd))
        {
            char c;
            exitPipe.read(&c,1);
            done = true;
        }
    }

    i3_evt * evt = loop.alloc_evt();
    evt->set_readdone();
    loop.enqueue_evt(evt);
}

/*virtual*/ void
i3_reader::send_stop(void)
{
    if (running())
    {
        char c = 1;
        exitPipe.write(&c,1);
    }
}
