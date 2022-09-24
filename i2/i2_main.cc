
// todo:
//   -Ir
//   -Iz
//   -zr
//   -zt
//   -f

#include "i2_options.h"
#include "posix_fe.h"
#include <netdb.h>

class i2_program
{
    i2_options  opts;
    bool _ok;
    pxfe_tcp_stream_socket * net_fd;
    uint64_t bytes_sent;
    uint64_t bytes_received;
    pxfe_timeval  start_time;
    pxfe_string  buffer;
    pxfe_ticker  ticker;
public:
    i2_program(int argc, char ** argv)
        : opts(argc, argv), _ok(false)
    {
        _ok = opts.ok;
        // debug only
        // opts.print();
        net_fd = NULL;
        bytes_received = 0;
        bytes_sent = 0;
    }
    ~i2_program(void)
    {
        // opts destructor will close the input & output fds.
        if (net_fd)
            delete net_fd;
    }
    bool ok(void) const { return _ok; }
    int main(void)
    {
        uint32_t addr;
        pxfe_errno e;
        if (opts.outbound &&
            pxfe_iputils::hostname_to_ipaddr(opts.hostname.c_str(),
                                             &addr) == false)
            return 1;
        if (opts.outbound)
        {
            net_fd = new pxfe_tcp_stream_socket;
            if (net_fd->init(&e) == false)
            {
                std::cerr << e.Format() << std::endl;
                return 1;
            }
            if (opts.verbose)
                fprintf(stderr, "connecting...");
            if (net_fd->connect(addr, opts.port_number, &e) == false)
            {
                std::cerr << e.Format() << std::endl;
                return 1;
            }
            if (opts.verbose)
                fprintf(stderr, "success\n");
        }
        else
        {
            pxfe_tcp_stream_socket listen;
            if (listen.init(opts.port_number,true,&e) == false)
            {
                std::cerr << e.Format() << std::endl;
                return 1;
            }
            listen.listen();
            if (opts.verbose)
                fprintf(stderr, "listening...");
            do {
                net_fd = listen.accept(&e);
                if (e.e != 0)
                    std::cerr << e.Format() << std::endl;
            } while (net_fd == NULL);
            if (opts.verbose)
            {
                uint32_t addr = net_fd->get_peer_addr();
                std::cerr << "accepted from "
                          << (int) ((addr >> 24) & 0xFF) << "."
                          << (int) ((addr >> 16) & 0xFF) << "."
                          << (int) ((addr >>  8) & 0xFF) << "."
                          << (int) ((addr >>  0) & 0xFF)
                          << std::endl;
            }
            // listen socket closed here, because we
            // no longer need it.
        }
        start_time.getNow();
        ticker.start(0, 500000);
        pxfe_poll p;

        p.set(net_fd->getFd(), POLLIN);
        p.set(ticker.fd(), POLLIN);

#define POLLERRS (POLLERR | POLLHUP | POLLNVAL)

        while (1)
        {
            int evt;

            if (opts.input_set)
                p.set(opts.input_fd, POLLIN);

            p.poll(1000);
            evt = p.rget(net_fd->getFd());

            if (evt & POLLIN)
            {
                if (!handle_net_fd())
                    break;
            }
            else if (evt & POLLERRS)
                break;

            if (opts.input_set)
            {
                evt = p.rget(opts.input_fd);
                if (evt & POLLIN)
                {
                    if (!handle_input_fd())
                        break;
                }
                else if (evt & POLLERRS)
                    break;
            }

            if (p.rget(ticker.fd()) & POLLIN)
                handle_tick();
        }
        ticker.pause();
        if (opts.verbose || opts.stats_at_end)
            print_stats(true);
        return 0;
    }
private:
    bool handle_net_fd(void)
    {
        pxfe_errno e;
        if (net_fd->recv(buffer, &e) == false)
        {
            std::cerr << e.Format() << std::endl;
            return false;
        }
        if (buffer.length() == 0)
            return false;
        bytes_received += buffer.length();
        if (opts.output_set)
        {
            int cc = -1;
            do  {
                cc = buffer.write(opts.output_fd);
                if (cc == buffer.length())
                    break;
                if (cc < 0)
                {
                    int e = errno;
                    char * err = strerror(e);
                    fprintf(stderr, "write failed: %d: %s\n", e, err);
                    return false;
                }
                else if (cc == 0)
                {
                    fprintf(stderr, "write returned zero\n");
                    return false;
                }
                else
                    // remove the bytes already written
                    // and go around again to get the rest.
                    buffer.erase(0,cc);
            } while (true);
        }
        return true;
    }
    bool handle_input_fd(void)
    {
        pxfe_errno e;
        int cc = buffer.read(opts.input_fd,
                             pxfe_tcp_stream_socket::MAX_MSG_LEN, &e);
        if (cc < 0)
        {
            std::cerr << e.Format() << std::endl;
            return false;
        }
        else if (cc == 0)
            return false;
        else
        {
            if (net_fd->send(buffer, &e) == false)
            {
                std::cerr << e.Format() << std::endl;
                return false;
            }
            bytes_sent += buffer.length();
        }
        return true;
    }
    void handle_tick(void)
    {
        ticker.doread();
        if (opts.verbose)
            print_stats(/*final*/false);
    }
    void print_stats(bool final)
    {
        pxfe_timeval now, diff;
        uint64_t total = bytes_sent + bytes_received;
        now.getNow();
        diff = now - start_time;
        float t = diff.usecs() / 1000000.0;
        if (t == 0.0)
            t = 99999.0;
        float bytes_per_sec = (float) total / t;
        float bits_per_sec = bytes_per_sec * 8.0;
        fprintf(stderr, "\r%" PRIu64 " in %u.%06u s "
               "(%.0f Bps %.0f bps)",
               total,
               (unsigned int) diff.tv_sec,
               (unsigned int) diff.tv_usec,
               bytes_per_sec, bits_per_sec);
        if (final)
            fprintf(stderr, "\n");
    }
};

extern "C" int
i2_main(int argc, char ** argv)
{
    i2_program  i2(argc, argv);
    if (i2.ok() == false)
        return 1;
    return i2.main();
}
