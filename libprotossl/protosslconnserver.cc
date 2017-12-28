
#include "libprotossl.h"
#include "bufprintf.h"

using namespace ProtoSSL;

ProtoSSLConnServer :: ProtoSSLConnServer(ProtoSSLMsgs * _msgs,
                                         int listeningPort)
    : msgs(_msgs)
{
    _ok = false;
    msgs->registerServer(this);
    Bufprintf<20> portString;
    portString.print("%d", listeningPort);
    mbedtls_net_init(&netctx);
    int ret = mbedtls_net_bind(&netctx, NULL, portString.getBuf(),
                               MBEDTLS_NET_PROTO_TCP);
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

    mbedtls_net_init(&new_netctx);

    ret = mbedtls_net_accept(&netctx, &new_netctx,
                             NULL, 0, NULL);
    if (ret == 0)
    {
        cnt = new ProtoSSLConnClient(msgs, new_netctx);
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

