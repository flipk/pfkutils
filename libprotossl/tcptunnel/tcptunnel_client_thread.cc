
#include "tcptunnel.h"

void
TcpTunnel :: start_client_thread(ProtoSSL::ProtoSSLConnClient * client,
                                 bool block)
{
    if (block)
    {
        // just call it
        clientthread_args  args;
        args.client = client;
        clientthread(&args);
    }
    else
    {
        clientthread_args * args = new clientthread_args;
        args->tcptun = this;
        args->func = &TcpTunnel::clientthread;
        args->client = client;

        start_a_thread(&clientthread_id,args);
    }
}

void
TcpTunnel :: clientthread(thread_args *_args)
{
    clientthread_args * args = (clientthread_args *) _args;
    client_dtlsq = new ProtoSSL::ProtoSslDtlsQueue(
        dtlsqConfig, args->client);

    pfk::tcptunnel::TunnelMessage_m  inbound;

    start_proxies();
    bool done = false;
    while (!done)
    {
        ProtoSSL::ProtoSslDtlsQueue::read_return_t ret;
        inbound.Clear();
        ret = client_dtlsq->handle_read(inbound);
        switch (ret)
        {
        case ProtoSSL::ProtoSslDtlsQueue::READ_MORE:
            // nothing to do, just go again
            break;

        case ProtoSSL::ProtoSslDtlsQueue::GOT_DISCONNECT:
            printf("GOT DISCONNECT\n");
            done = true;
            break;

        case ProtoSSL::ProtoSslDtlsQueue::GOT_TIMEOUT:
            printf("GOT TIMEOUT\n");
            done = true;
            break;

        case ProtoSSL::ProtoSslDtlsQueue::LINK_DOWN:
            printf("LINK DOWN\n");
            // xxx what to do
            break;

        case ProtoSSL::ProtoSslDtlsQueue::LINK_UP:
            printf("LINK UP\n");
            // xxx what to do
            break;

        case ProtoSSL::ProtoSslDtlsQueue::GOT_MESSAGE:
            printf("GOT MESSAGE :\n%s\n", inbound.DebugString().c_str());
            handle_tunnel_message(inbound);
            break;
        }
    }
    stop_proxies();
    delete client_dtlsq;
}
