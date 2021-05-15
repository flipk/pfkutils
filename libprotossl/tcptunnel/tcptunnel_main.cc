
#include "tcptunnel.h"
#include "certs.h"

int TcpTunnel :: main(void)
{
    if (mode == SERVER_MODE)
    {
        printf("server mode on port %u\n", peer_port);
    }
    else
    {
        printf("client mode, to ip %08x port %u\n",
               peer_ip, peer_port);
    }
    printf("%d proxies\n", proxy_ports.size());
    for (size_t ind = 0; ind < proxy_ports.size(); ind++)
    {
        proxy_info &pi = proxy_ports[ind];
        printf("local port %u -> remote ip %08x port %u\n",
               pi.local_port, pi.remote_ip, pi.remote_port);
    }

    ProtoSSL::ProtoSSLCertParams   certParams(
        root_CA_crt, client_crt, client_key, "" /*password*/ );

    msgs = new ProtoSSL::ProtoSSLMsgs(
        false,  // _nonBlockingMode
        false,  // _debugFlag
        0,      // read_timeout
        false,  // use_tcp
        1000,   // dtls_timeout_min
        2000 ); // dtls_timeout_max

    dtlsqConfig.ticks_per_second = 10;
    dtlsqConfig.hearbeat_interval = 10; // 1 second
    dtlsqConfig.max_missed_heartbeats = 3; // 3 seconds
    dtlsqConfig.add_queue(
        dtlsq_num, /*reliable*/ true,
        ProtoSSL::ProtoSslDtlsQueueConfig::FIFO,
        ProtoSSL::ProtoSslDtlsQueueConfig::NONE, // limit
        0); // no limit

    if (msgs->loadCertificates(certParams) == false)
    {
        printf("FAILURE loading certificates\n");
        return 1;
    }

    if (msgs->validateCertificates() == false)
    {
        printf("FAILURE VALIDATING certificates\n");
        return 1;
    }

    if (mode == SERVER_MODE)
    {
        ProtoSSL::ProtoSSLConnServer * server =
            msgs->startServer((int) peer_port);

        if (server == NULL)
        {
            printf("FAILURE creating server\n");
            return 1;
        }

        printf("WAITING FOR CONNECTION\n");
        while (1)
        {
            ProtoSSL::ProtoSSLConnClient * client =
                server->handle_accept();
            if (client)
            {
                if (client->ok())
                    start_client_thread(client, /*block*/false);
                else
                {
                    printf("CLIENT not ok\n");
                    delete client;
                }
            }
        }
    }
    else
    {
        ProtoSSL::ProtoSSLConnClient * client =
            msgs->startClient(peer_host, (int) peer_port);
        if (client == NULL)
        {
            printf("FAILURE creating client\n");
            return 1;
        }
        if (client->ok() == false)
        {
            printf("CLIENT not ok\n");
            return 1;
        }
        start_client_thread(client, /*block*/true);
    }

    return 0;
}
