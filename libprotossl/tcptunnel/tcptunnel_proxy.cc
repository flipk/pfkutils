
#include "tcptunnel.h"

void
TcpTunnel :: start_proxies(void)
{
    acceptorthread_args * args = new acceptorthread_args;
    args->tcptun = this;
    args->func = &TcpTunnel::acceptorthread;
    start_a_thread(&acceptorthread_id,args);
}

void
TcpTunnel :: stop_proxies(void)
{
    char c = 1;
    acceptor_closer_pipe.write(&c, 1);
    pthread_join(acceptorthread_id,NULL);
    acceptor_closer_pipe.read(&c, 1);
}

void
TcpTunnel :: acceptorthread(thread_args *_args)
{
    acceptorthread_args * args = (acceptorthread_args *) _args;

    for (size_t ind = 0; ind < proxy_ports.size(); ind++)
    {
        proxy_info &pi = proxy_ports[ind];
        pxfe_errno e;
        if (pi.sock.init((short)pi.local_port, true, &e) == false)
        {
            printf("listen to port %u : %s\n",
                   pi.local_port, e.Format().c_str());
            pi.sock.close();
        }
        else
        {
            pi.sock.listen();
            printf("listening to port %u\n", pi.local_port);
        }
    }

    bool done = false;
    while (!done)
    {
        pxfe_select  sel;
        sel.rfds.set(acceptor_closer_pipe.readEnd);
        for (size_t ind = 0; ind < proxy_ports.size(); ind++)
        {
            int fd = proxy_ports[ind].sock.getFd();
            if (fd != -1)
                sel.rfds.set(fd);
        }
        sel.tv.set(1,0);
        sel.select();
        if (sel.rfds.is_set(acceptor_closer_pipe.readEnd))
            // select on closer, but don't read!
            done = true;
        for (size_t ind = 0; ind < proxy_ports.size(); ind++)
        {
            int fd = proxy_ports[ind].sock.getFd();
            if (fd != -1 && sel.rfds.is_set(fd))
                accept_proxy(ind);
        }
    }

    for (size_t ind = 0; ind < proxy_ports.size(); ind++)
        proxy_ports[ind].sock.close();
}

void
TcpTunnel :: accept_proxy(int proxy_index)
{
    proxy_info &pi = proxy_ports[proxy_index];

    pxfe_errno e;
    pxfe_tcp_stream_socket * newsock = pi.sock.accept(&e);
    if (newsock == NULL)
    {
        printf("accept on port %u: %s\n",
               pi.local_port, e.Format().c_str());
    }
    else
    {
        printf("accepted new conn on port %u\n", pi.local_port);
        proxythread_args * args = new proxythread_args;
        args->tcptun = this;
        args->func = &TcpTunnel::proxythread;
        args->proxy_index = proxy_index;
        args->newsock = newsock;
        start_a_thread(&proxythread_id,args);
    }
}

void
TcpTunnel :: proxythread(thread_args *_args)
{
    proxythread_args * args = (proxythread_args *) _args;
    proxy_info &pi = proxy_ports[args->proxy_index];


    // xxxxxxxxxxxxxxxxxx
    printf("forcing closed new sock on port %u\n", pi.local_port);
    delete args->newsock;



#if 0
    pfk::tcptunnel::TunnelMessage_m  outbound;

    outbound.set_type(pfk::tcptunnel::ESTABLISHED);
    ProtoSSL::ProtoSslDtlsQueue::send_return_t sendret =
        client_dtlsq->send_message(dtlsq_num,outbound);
    printf("send return = %d\n", sendret);
    outbound.Clear();
#endif

}



void
TcpTunnel :: handle_tunnel_message(
    const pfk::tcptunnel::TunnelMessage_m &inbound)
{
    // xxxxxxxxxxxxxxxxxx
}
