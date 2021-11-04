
#include <iostream>
#include <list>
#include <vector>
#include <stdarg.h>
#include <signal.h>
#include "posix_fe.h"
#include "thread_slinger.h"

using namespace std;

#define TICKS_PER_SECOND 20

struct Options {
    Options(int argc, char ** argv) {
        ok = false;
        if ((argc < 6) || (((argc - 1) % 5) != 0))
        {
        usage:
            cerr << "usage: slowlink [bytes_per_second latency_in_mS "
                    "listen_port remote_host remote_port] (repeated)\n";
            return;
        }
        for (int ind = 1; ind < argc; ind += 5)
        {
            params p;
            if (pxfe_utils::parse_number(argv[ind+0],
                                         &p.bytes_per_second) == false)
            {
                cerr << "error parsing bytes_per_second\n";
                goto usage;
            }
            if (pxfe_utils::parse_number(argv[ind+1],
                                         &p.latency_in_ms) == false)
            {
                cerr << "error parsing latency_in_ms\n";
                goto usage;
            }
            if (pxfe_iputils::parse_port_number(argv[ind+2],
                                                &p.listen_port) == false)
            {
                cerr << "error parsing listen_port\n";
                goto usage;
            }
            if (pxfe_iputils::hostname_to_ipaddr(argv[ind+3],
                                                 &p.remote_addr) == false)
            {
                cerr << "error parsing remote_host\n";
                goto usage;
            }
            if (pxfe_iputils::parse_port_number(argv[ind+4],
                                                &p.remote_port) == false)
            {
                cerr << "error parsing remote_port\n";
                goto usage;
            }
            p.latency.set(p.latency_in_ms / 1000,
                          (p.latency_in_ms % 1000) * 1000);
            param_list.push_back(p);
        }
        ok = true;
    }
    void print(void) const {
        cout << param_list.size() << " configs:\n";
        for (auto &p : param_list)
            p.print();
    }
    bool ok;
    struct params {
        uint32_t bytes_per_second;
        uint32_t latency_in_ms;
        pxfe_timeval latency;
        uint16_t listen_port;
        uint32_t remote_addr;
        uint16_t remote_port;
        void print(void) const {
            cout << bytes_per_second << " bps, delay "
                 << latency_in_ms << " ms from "
                 << listen_port << " to "
                 << pxfe_iputils::format_ip(remote_addr) << ":"
                 << remote_port << "\n";
        }
    };
    vector<params> param_list;
    typedef vector<params>::const_iterator iter;
    iter begin(void) const { return param_list.begin(); }
    iter end(void) const { return param_list.end(); }
};

class Slowlink {
    const Options &opts;
    struct config {
        config(const Options::params &_p)
            : p(_p) { }
        const Options::params &p;
        pxfe_tcp_stream_socket s;
    };
    typedef list<config*> configs_t;
    typedef configs_t::iterator configiter_t;
    configs_t configs;
    pxfe_timeval last_stats;
    uint64_t total_bytes;
    struct holding_buffer : public ThreadSlinger::thread_slinger_message {
        pxfe_string buf;
        pxfe_timeval send_time; // dont send until after this time.
        bool ready;
        holding_buffer(void) { ready = false; }
        void init(void) { ready = false; }
        void cleanup(void) { ready = false; }
    };
    static const int holding_pool_initial_size = 1000;
    ThreadSlinger::thread_slinger_pool<holding_buffer>  holding_pool;
    struct connection {
        connection(const Options::params &_p,
                   pxfe_tcp_stream_socket *_in_s)
            : p(_p), in_s(_in_s)
        {
            tokens_per_tick = p.bytes_per_second / TICKS_PER_SECOND;
            tokens_out = tokens_in = tokens_per_tick;
            ok = true;
            read_done = false;
        }
        ~connection(void) {
            delete in_s;
        }
        const Options::params &p;
        pxfe_tcp_stream_socket *in_s;
        pxfe_tcp_stream_socket out_s;
        ThreadSlinger::thread_slinger_queue<holding_buffer>  in2out;
        ThreadSlinger::thread_slinger_queue<holding_buffer>  out2in;
        int tokens_per_tick;
        int tokens_out;
        int tokens_in;
        bool ok;
        bool read_done;
    };
    typedef list<connection*> conns_t;
    typedef conns_t::iterator conniter_t;
    conns_t   conns;
public:
    Slowlink(const Options &_opts) : opts(_opts) {
        holding_pool.add(holding_pool_initial_size);
        debug = getenv("DEBUG_SLOWLINK") != NULL;
        last_stats.getNow();
        total_bytes = 0;
    }
    ~Slowlink(void) {
        configiter_t it1 = configs.begin();
        while (it1 != configs.end())
        {
            delete *it1;
            it1 = configs.erase(it1);
        }
        conniter_t it2 = conns.begin();
        while (it2 != conns.end())
        {
            delete *it2;
            it2 = conns.erase(it2);
        }
    }
    int main(void) {
        // first create all the sockets.
        for ( auto &p : opts )
        {
            p.print();
            config * c = new config(p);
            pxfe_errno e;
            if (c->s.init(p.listen_port, true, &e) == false)
            {
                cerr << "init listening socket: " << e.Format() << endl;
                return 1;
            }
            configs.push_back(c);
        }
        // now allow them all to listen.
        for ( auto c : configs )
            c->s.listen();
        pxfe_ticker  ticker;
        pxfe_timeval select_start, select_end, select_diff;
        ticker.start(0, 1000000 / TICKS_PER_SECOND);
        while (1)
        {
            pxfe_select  sel;
            sel.rfds.set(ticker.fd());
            for (auto c : configs)
            {
                printlog("sel.rfds.set config %d", c->s.getFd());
                sel.rfds.set(c->s.getFd());
            }
            printlog("holding_pool.get_count() = %d",
                     holding_pool.get_count());
            for (auto c : conns)
            {
                if (holding_pool.get_count() >= conns.size() &&
                    c->read_done == false)
                {
                    if (in_space(c))
                    {
                        printlog("sel.rfds.set in %d in conn %" PRIuPTR,
                                 c->in_s->getFd(), (uintptr_t) c);
                        sel.rfds.set(c->in_s->getFd());
                    }
                    if (out_space(c))
                    {
                        printlog("sel.rfds.set out %d in conn %" PRIuPTR,
                                 c->out_s.getFd(), (uintptr_t) c);
                        sel.rfds.set(c->out_s.getFd());
                    }
                }
                if (is_out_ready(c))
                {
                    printlog("sel.wfds.set out %d in conn %" PRIuPTR,
                             c->out_s.getFd(), (uintptr_t) c);
                    sel.wfds.set(c->out_s.getFd());
                }
                if (is_in_ready(c))
                {
                    printlog("sel.wfds.set in %d in conn %" PRIuPTR,
                             c->in_s->getFd(), (uintptr_t) c);
                    sel.wfds.set(c->in_s->getFd());
                }
            }
            sel.tv.set(1,0);
            if (debug)
                select_start.getNow();
            sel.select();
            if (debug)
            {
                select_end.getNow();
                select_diff = select_end - select_start;
                printlog("was in select %d ms", select_diff.msecs());
            }
            if (sel.rfds.is_set(ticker.fd()))
                handle_tick(ticker);
            for (auto c : configs)
            {
                if (sel.rfds.is_set(c->s.getFd()))
                    handle_accept(c);
            }
            for (auto c : conns)
            {
                if (sel.rfds.is_set(c->in_s->getFd()))
                    handle_read_in(c);
                if (sel.wfds.is_set(c->in_s->getFd()))
                    handle_write_in(c);
                if (sel.rfds.is_set(c->out_s.getFd()))
                    handle_read_out(c);
                if (sel.wfds.is_set(c->out_s.getFd()))
                    handle_write_out(c);
            }
            for (conniter_t it = conns.begin();
                 it != conns.end();)
            {
                auto c = *it;
                if (c->ok)
                    it++;
                else
                {
                    printlog("erasing conn at %" PRIuPTR,
                             (uintptr_t) c);
                    it = conns.erase(it);
                    delete c;
                }
            }
        }
        ticker.pause();
        return 0;
    }
    bool debug;
private:
    void printlog(const char *format, ...) {
        if (!debug)
            return;
        va_list ap;
        va_start(ap,format);
        pxfe_timeval tv;
        tv.getNow();
        printf("%s ", tv.Format("%s").c_str());
        vprintf(format, ap);
        printf("\n");
        va_end(ap);
    }
    void handle_tick(pxfe_ticker &ticker) {
        pxfe_timeval now;
        holding_buffer * b;
        now.getNow();
        ticker.doread();
        printlog("tick: %s", now.Format().c_str());
        for (auto c : conns)
        {
            // first do out q
            b = c->in2out.get_head();
            if (b && now > b->send_time)
            {
                b->ready = true;
                c->tokens_out += c->tokens_per_tick;
                printlog("conn %" PRIuPTR " buf %" PRIuPTR " ready, "
                         "tokens_out = %d",
                         (uintptr_t) c,
                         (uintptr_t) b,
                         c->tokens_out);
            }
            else
                c->tokens_out = c->tokens_per_tick;
            // next do in q
            b = c->out2in.get_head();
            if (b && now > b->send_time)
            {
                b->ready = true;
                c->tokens_in += c->tokens_per_tick;
                printlog("conn %" PRIuPTR " buf %" PRIuPTR " ready, "
                         "tokens_in = %d", c->tokens_out,
                         (uintptr_t) c,
                         (uintptr_t) b,
                         c->tokens_out);
            }
            else
                c->tokens_in = c->tokens_per_tick;
        }
        pxfe_timeval d = now - last_stats;
        if (d.msecs() > 1000)
        {
            printf("\r %d conns %" PRIu64 " bytes         ",
                   (int) conns.size(), total_bytes);
            if (debug)
                printf("\n");
            else
                fflush(stdout);
            last_stats = now;
        }
    }
    void handle_accept(config *c) {
        pxfe_tcp_stream_socket *ns;
        pxfe_errno e;
        printlog("accept on %d", c->s.getFd());
        ns = c->s.accept(&e);
        if (e.e != 0)
        {
            cerr << e.Format() << endl;
            printlog("accept error %d", e.e);
        }
        if (ns)
        {
            connection * co = new connection(c->p, ns);
            co->out_s.init();
            if (co->out_s.connect(c->p.remote_addr,
                                  c->p.remote_port, &e) == false)
            {
                cerr << e.Format() << endl;
                delete co;
            }
            else
            {
                conns.push_back(co);
                printlog("accept new conn at %" PRIuPTR, (uintptr_t) co);
            }
        }
        else
        {
            printlog("accept returns null");
        }
    }
    bool in_space(connection *c) {
        printlog("conn %" PRIuPTR " in count %d",
                 (uintptr_t) c, c->in2out.get_count());
        if (c->in2out.get_count() < 10)
            return true;
        return false;
    }
    bool out_space(connection *c) {
        printlog("conn %" PRIuPTR " out count %d",
                 (uintptr_t) c, c->out2in.get_count());
        if (c->out2in.get_count() < 10)
            return true;
        return false;
    }
    void handle_read_in(connection *c) {
        holding_buffer * b = holding_pool.alloc();
        if (b == NULL)
        {
            printlog("read_in: holding pool empty");
            return;
        }
        pxfe_errno e;
        if (c->in_s->read(b->buf, c->tokens_per_tick, &e) == false)
            cerr << e.Format() << endl;
        printlog("read on %d err %d size %d in buffer %" PRIuPTR,
                 c->in_s->getFd(), e.e, b->buf.size(),
                 (uintptr_t) b);
        if (b->buf.size() == 0)
        {
            c->read_done = true;
            holding_pool.release(b);
            return;
        }
        b->send_time.getNow();
        b->send_time += c->p.latency;
        printlog("got buf of size %d, send time %s(%s)",
                 b->buf.size(),
                 b->send_time.Format("%s").c_str(),
                 b->send_time.Format().c_str());
        c->in2out.enqueue(b);
    }
    void handle_read_out(connection *c) {
        holding_buffer * b = holding_pool.alloc();
        if (b == NULL)
        {
            printlog("read_in: holding pool empty");
            return;
        }
        pxfe_errno e;
        if (c->out_s.read(b->buf, c->tokens_per_tick, &e) == false)
            cerr << e.Format() << endl;
        printlog("read on %d err %d size %d in buffer %" PRIuPTR,
                 c->out_s.getFd(), e.e, b->buf.size(),
                 (uintptr_t) b);
        if (b->buf.size() == 0)
        {
            c->read_done = true;
            holding_pool.release(b);
            return;
        }
        b->send_time.getNow();
        b->send_time += c->p.latency;
        printlog("got buf of size %d, send time %s(%s)",
                 b->buf.size(),
                 b->send_time.Format("%s").c_str(),
                 b->send_time.Format().c_str());
        c->out2in.enqueue(b);
    }
    bool is_out_ready(connection *c) {
        if (c->read_done && c->out2in.empty() && c->in2out.empty())
        {
            printlog("conn %" PRIuPTR " read done q empty, closing",
                     (uintptr_t) c);
            c->ok = false;
        }
        holding_buffer * b = c->in2out.get_head();
        if (b && b->ready && c->tokens_out >= b->buf.size())
            return true;
        return false;
    }
    bool is_in_ready(connection *c) {
        holding_buffer * b = c->out2in.get_head();
        if (b && b->ready && c->tokens_in >= b->buf.size())
            return true;
        return false;
    }
    void handle_write_in(connection *c) {
        holding_buffer * b = c->out2in.dequeue();
        if (b == NULL)
        {
            printlog("write_in: dequeue failed");
            return;
        }
        pxfe_errno e;
        if (c->in_s->send(b->buf, &e) == false)
        {
            cerr << e.Format() << endl;
            c->ok = false;
        }
        printlog("write_in: sending %d bytes to %d from buf %" PRIuPTR,
                 b->buf.size(), c->in_s->getFd(),
                 (uintptr_t) b);
        c->tokens_in -= b->buf.size();
        total_bytes += b->buf.size();
        holding_pool.release(b);
    }
    void handle_write_out(connection *c) {
        holding_buffer * b = c->in2out.dequeue();
        if (b == NULL)
        {
            printlog("write_in: dequeue failed");
            return;
        }
        pxfe_errno e;
        if (c->out_s.send(b->buf, &e) == false)
        {
            cerr << e.Format() << endl;
            c->ok = false;
        }
        printlog("write_out: sending %d bytes to %d from buf %" PRIuPTR,
                 b->buf.size(), c->out_s.getFd(),
                 (uintptr_t) b);
        c->tokens_out -= b->buf.size();
        total_bytes += b->buf.size();
        holding_pool.release(b);
    }
};

static void
block_sigpipe(void)
{
    struct sigaction act;
    sigemptyset(&act.sa_mask);
    act.sa_handler = SIG_IGN;
    act.sa_flags = SA_RESTART;
    sigaction(SIGPIPE, &act, NULL);
}

extern "C" int
slowlink_main(int argc, char ** argv)
{
    Options opts(argc, argv);
    if (!opts.ok)
        return 1;
//    opts.print();
    Slowlink prog(opts);
    block_sigpipe();
    return prog.main();
}
