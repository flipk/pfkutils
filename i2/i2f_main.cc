
#include "i2f_options.h"
#include <sstream>

using namespace std;

class i2f_program
{
    i2f_options  opts;
    typedef i2f_options::forw_port    forw_port;
    typedef i2f_options::ports_iter_t ports_iter_t;
    bool _ok;
    struct conn {
        conn(void)
        {
            port = NULL;
            client_socket = NULL;
        }
        ~conn(void)
        {
            if (client_socket)
                delete client_socket;
        }
        forw_port              * port;
        pxfe_tcp_stream_socket * client_socket;
        pxfe_tcp_stream_socket   server_socket;
    };
    typedef std::list<conn*> connlist_t;
    typedef connlist_t::iterator conn_iter_t;
    connlist_t conns;
    pxfe_ticker ticker;
    uint64_t  bytes;
public:
    i2f_program(int argc, char ** argv)
        : opts(argc, argv), bytes(0)
    {
        if (opts.ok == false)
        {
            _ok = false;
            return;
        }

        ports_iter_t  it;
        pxfe_errno e;
        for (it = opts.ports.begin(); it != opts.ports.end(); it++)
        {
            forw_port * p = *it;
            if (p->listen_socket.init(p->port,true,&e) == false)
            {
                std::cerr << e.Format() << std::endl;
                return;
            }
            p->listen_socket.listen();
        }

        _ok = true;
    }
    bool ok(void) const { return _ok; }
    ~i2f_program(void)
    {
        conn_iter_t it;
        for (it = conns.begin(); it != conns.end(); )
        {
            conn * c = *it;
            it = conns.erase(it);
            delete c;
        }
    }
    int main(void)
    {
        if (opts.debug_flag)
            opts.print();

        ports_iter_t pit;
        conn_iter_t  cit;

        for (pit = opts.ports.begin(); pit != opts.ports.end(); pit++)
        {
            forw_port * p = *pit;
            p->listen_socket.listen();
        }

        ticker.start(0,500000);

        bool done = false;
        while (!done)
        {
            pxfe_select sel;

            for (pit = opts.ports.begin(); pit != opts.ports.end(); pit++)
            {
                forw_port * p = *pit;
                sel.rfds.set(p->listen_socket.getFd());
            }
            for (cit = conns.begin(); cit != conns.end(); cit++)
            {
                conn * c = *cit;
                sel.rfds.set(c->client_socket->getFd());
                sel.rfds.set(c->server_socket.getFd());
            }
            sel.rfds.set(ticker.fd());

            sel.tv.set(10,0);
            sel.select();

            for (pit = opts.ports.begin(); pit != opts.ports.end(); pit++)
            {
                forw_port * p = *pit;
                if (sel.rfds.is_set(p->listen_socket.getFd()))
                    handle_accept(p);
            }
            if (done)
                break;
            for (cit = conns.begin(); cit != conns.end();)
            {
                conn * c = *cit;
                bool del = false;
                if (sel.rfds.is_set(c->client_socket->getFd()))
                    if (handle_client(c) == false)
                        del = true;
                if (sel.rfds.is_set(c->server_socket.getFd()))
                    if (handle_server(c) == false)
                        del = true;
                if (del == false)
                    cit++;
                else
                {
                    cit = conns.erase(cit);
                    if (opts.verbose)
                    {
                        uint32_t addr = c->client_socket->get_peer_addr();
                        fprintf(stderr, "\nconnection from "
                                "%d.%d.%d.%d closed\n",
                                (addr >> 24) & 0xFF, (addr >> 16) & 0xFF,
                                (addr >>  8) & 0xFF, (addr >>  0) & 0xFF);
                    }
                    delete c;
                }
            }
            if (sel.rfds.is_set(ticker.fd()))
                handle_tick();
        }
        ticker.pause();
        print_stats(/*final*/true);
        return 0;
    }
private:
    void handle_accept(forw_port * p)
    {
        pxfe_errno e;
        pxfe_tcp_stream_socket * s = p->listen_socket.accept(&e);
        if (!s)
        {
            cerr << e.Format() << endl;
            return;
        }
        if (opts.verbose)
        {
            uint32_t addr = s->get_peer_addr();
            fprintf(stderr, "\nnew connection from %d.%d.%d.%d\n",
                    (addr >> 24) & 0xFF, (addr >> 16) & 0xFF,
                    (addr >>  8) & 0xFF, (addr >>  0) & 0xFF);
        }
        conn * c = new conn;
        c->port = p;
        c->client_socket = s;
        if (c->server_socket.init(&e) == false)
            cerr << e.Format() << endl;
        bool good = 
            c->server_socket.connect(p->remote_addr, p->remote_port, &e);

        if (good)
            conns.push_back(c);
        else
        {
            cerr << e.Format() << endl;
            delete c;
        }
    }
    pxfe_string  buffer;
    ostringstream  dbg;
    bool handle_client(conn * c)
    {
        pxfe_errno e;
        if (opts.debug_flag)
        {
            dbg.str().clear();
            dbg << "reading from client: ";
        }
        if (c->client_socket->recv(buffer, &e) == false)
        {
            if (opts.debug_flag)
            {
                dbg << e.Format();
                cerr << dbg.str();
            }
            return false;
        }
        if (buffer.length() == 0)
        {
            if (opts.debug_flag)
            {
                dbg << "got end of stream, closing\n";
                cerr << dbg.str();
            }
            return false;
        }
        if (opts.debug_flag)
        {
            dbg << "got " << buffer.length() << " bytes, writing\n";
            cerr << dbg.str();
        }
        bytes += buffer.length();
        bool sendret = c->server_socket.send(buffer, &e);
        if (!sendret)
            cerr << e.Format() << endl;
        return sendret;
    }
    bool handle_server(conn * c)
    {
        pxfe_errno e;
        if (opts.debug_flag)
        {
            dbg.str().clear();
            dbg << "reading from server: ";
        }
        if (c->server_socket.recv(buffer, &e) == false)
        {
            if (opts.debug_flag)
            {
                dbg << e.Format();
                cerr << dbg.str();
            }
            return false;
        }
        if (buffer.length() == 0)
        {
            if (opts.debug_flag)
            {
                dbg << "got end of stream, closing\n";
                cerr << dbg.str();
            }
            return false;
        }
        if (opts.debug_flag)
        {
            dbg << "got " << buffer.length() << " bytes, writing\n";
            cerr << dbg.str();
        }
        bytes += buffer.length();
        bool sendret = c->client_socket->send(buffer, &e);
        if (!sendret)
            cerr << e.Format() << endl;
        return sendret;
    }
    void handle_tick(void)
    {
        buffer.read(ticker.fd(), 100);
        print_stats(/*final*/false);
    }
    void print_stats(bool final)
    {
        if (opts.verbose || (final && opts.stats_at_end))
            fprintf(stderr, "\r%" PRIu64 " bytes ", bytes);
        if (final)
            fprintf(stderr,"\n");
    }
};

extern "C" int
i2f_main(int argc, char ** argv)
{
    i2f_program  i2f(argc, argv);
    if (i2f.ok() == false)
        return 1;
    return i2f.main();
}
