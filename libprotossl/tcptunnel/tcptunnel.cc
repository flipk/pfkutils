
#include "tcptunnel.h"
#include "tuples.h"

TcpTunnel :: TcpTunnel(int argc, char ** argv)
{
    tuple2_regex  t2r;
    tuple3_regex  t3r;
    _ok = false;
    if (argc < 2)
        // no local or remote specs, booo
        return;
    if (t2r.parse_tuple2(argv[1]) == false)
    {
        printf("arg 1 parse error\n");
        return;
    }
    if (t2r.is_local)
    {
        mode = SERVER_MODE;
        peer_ip = 0;
        peer_port = t2r.local_port;
    }
    else
    {
        mode = CLIENT_MODE;
        peer_ip = t2r.remote_ip;
        peer_host = pxfe_iputils::format_ip(peer_ip);
        peer_port = t2r.remote_port;
    }

    for (int ind = 2; ind < argc; ind++)
    {
        std::string arg(argv[ind]);
        if (t3r.parse_tuple3(arg) == false)
        {
            printf("arg %d parse error\n", ind);
            return;
        }
        proxy_info  pi;
        pi.local_port = t3r.local_port;
        pi.remote_ip = t3r.remote_ip;
        pi.remote_port = t3r.remote_port;
        proxy_ports.push_back(pi);
    }
    
    _ok = true;
}

TcpTunnel :: ~TcpTunnel(void)
{
    if (_ok == false)
        // do nothing, it never initialized right anyway.
        return;

    // xxx need to kill threads

    if (msgs)
        delete msgs;
}

void
TcpTunnel :: usage(void)
{
    printf(
"usage: \n"
"tcptunnel local_port            [local_port:remote_ip:remote_port] ...\n"
"tcptunnel remote_ip:remote_port [local_port:remote_ip:remote_port] ...\n"
        );
}




// force linking dll3.o from libextrautils.a
// because Makefile.inc puts DEPLIBS before LIBS,
// and only libprotossl.a needs dll3.o,
// so it won't get linked otherwise.
int ______dummy(void) { return DLL3::dll3_hash_primes[0]; }
