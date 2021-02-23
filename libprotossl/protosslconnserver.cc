
#include "libprotossl.h"
#include "bufprintf.h"

using namespace ProtoSSL;

ProtoSSLConnServer :: ProtoSSLConnServer(ProtoSSLMsgs * _msgs,
                                         int listeningPort,
                                         bool _use_tcp)
    : msgs(_msgs), use_tcp(_use_tcp)
{
    _ok = false;
    Bufprintf<20> portString;
    portString.print("%d", listeningPort);
    mbedtls_net_init(&netctx);
    int ret = mbedtls_net_bind(&netctx, NULL, portString.getBuf(),
                               use_tcp ?
                               MBEDTLS_NET_PROTO_TCP :
                               MBEDTLS_NET_PROTO_UDP );
    if (ret == 0)
    {
        if (msgs->nonBlockingMode)
            mbedtls_net_set_nonblock(&netctx);
        _ok = true;
    }
    else
    {
        char strbuf[200];
        mbedtls_strerror( ret, strbuf, sizeof(strbuf));
        fprintf(stderr,"net bind returned 0x%x: %s\n", -ret, strbuf);
    }
    msgs->registerServer(this);
}

ProtoSSLConnServer :: ~ProtoSSLConnServer(void)
{
    msgs->unregisterServer(this);
    mbedtls_net_free(&netctx);
}

ProtoSSLConnClient *
ProtoSSLConnServer :: handle_accept(void)
{
    int ret;
    ProtoSSLConnClient * cnt = NULL;
    mbedtls_net_context new_netctx;
    unsigned char client_ip[16] = { 0 };
    size_t cliip_len;

    mbedtls_net_init(&new_netctx);

    ret = mbedtls_net_accept(&netctx, &new_netctx,
                             client_ip, sizeof( client_ip ), &cliip_len);
    if (ret == 0)
    {
        cnt = new ProtoSSLConnClient(msgs, new_netctx, use_tcp,
                                     client_ip, cliip_len);
        if (cnt->ok() == false)
        {
            delete cnt;
            cnt = NULL;
        }
    }
    else
    {
        fprintf(stderr,"accept returned shit\n");
        mbedtls_net_free(&new_netctx);
    }

    return cnt;
}
