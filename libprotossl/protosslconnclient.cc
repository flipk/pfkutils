
#include "libprotossl.h"
#include "bufprintf.h"

using namespace ProtoSSL;

ProtoSSLConnClient :: ProtoSSLConnClient(ProtoSSLMsgs * _msgs,
                                         mbedtls_net_context new_netctx)
    : msgs(_msgs)
{
    _ok = false;
    send_close_notify = false;
    ssl_initialized = false;
    msgs->registerClient(this);
    netctx = new_netctx;
    WaitUtil::Lock lock(&ssl_lock);
    _ok = init_common();
}

ProtoSSLConnClient :: ProtoSSLConnClient(ProtoSSLMsgs * _msgs,
                                         const std::string &remoteHost,
                                         int remotePort)
    : msgs(_msgs)
{
    _ok = false;
    send_close_notify = false;
    ssl_initialized = false;
    msgs->registerClient(this);

    WaitUtil::Lock lock(&ssl_lock);
    Bufprintf<20> portString;
    portString.print("%d", remotePort);
    mbedtls_net_init(&netctx);
    int ret = mbedtls_net_connect(&netctx, remoteHost.c_str(),
                                  portString.getBuf(),
                                  MBEDTLS_NET_PROTO_TCP);
    if (ret != 0)
    {
        char strbuf[200];
        mbedtls_strerror( ret, strbuf, sizeof(strbuf));
        printf("net bind returned 0x%x: %s\n", -ret, strbuf);
        return;
    }

    _ok = init_common();
}

// this function requires ssl_lock to be locked.
bool
ProtoSSLConnClient :: init_common(void)
{
    int ret;

    mbedtls_ssl_init( &sslctx );
    ssl_initialized = true;
    mbedtls_ssl_setup( &sslctx, &msgs->sslcfg );
    mbedtls_ssl_set_hs_authmode( &sslctx, MBEDTLS_SSL_VERIFY_REQUIRED );
    mbedtls_ssl_set_bio( &sslctx, &netctx,
                 &mbedtls_net_send, &mbedtls_net_recv,
                 &mbedtls_net_recv_timeout);

    while ((ret = mbedtls_ssl_handshake( &sslctx )) != 0)
    {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ &&
            ret != MBEDTLS_ERR_SSL_WANT_WRITE )
        {
            char strbuf[200];
            mbedtls_strerror( ret, strbuf, sizeof(strbuf));
            printf( " failed\n  ! ssl_handshake returned 0x%x: %s\n\n",
                    -ret, strbuf );
            return false;
        }
    }

    if( ( ret = mbedtls_ssl_get_verify_result( &sslctx ) ) != 0 )
    {
        printf( "ssl_get_verify_result failed (ret = 0x%x)\n", ret );
        if( ( ret & MBEDTLS_X509_BADCERT_EXPIRED ) != 0 )
            printf( "  ! server certificate has expired\n" );
        if( ( ret & MBEDTLS_X509_BADCERT_REVOKED ) != 0 )
            printf( "  ! server certificate has been revoked\n" );
        if( ( ret & MBEDTLS_X509_BADCERT_CN_MISMATCH ) != 0 )
            printf( "  ! CN mismatch (expected CN=%s)\n",
                    "PolarSSL Server 1" );
        if( ( ret & MBEDTLS_X509_BADCERT_NOT_TRUSTED ) != 0 )
            printf( "  ! self-signed or not signed by a trusted CA\n" );
        printf( "\n" );
        return false;
    }

    if (msgs->nonBlockingMode)
        mbedtls_net_set_nonblock(&netctx);
    send_close_notify = true;
    return true;
}

ProtoSSLConnClient :: ~ProtoSSLConnClient(void)
{
    WaitUtil::Lock lock(&ssl_lock);
    mbedtls_net_set_block(&netctx);
    msgs->unregisterClient(this);
    if (send_close_notify)
        mbedtls_ssl_close_notify( &sslctx );
    mbedtls_net_free(&netctx);
    if (ssl_initialized)
        mbedtls_ssl_free(&sslctx);
}

ProtoSSLConnClient :: read_return_t
ProtoSSLConnClient :: handle_read(MESSAGE &msg)
{
    int ret;
    WaitUtil::Lock lock(&ssl_lock);

    rcvbuf.resize(MBEDTLS_SSL_MAX_CONTENT_LEN);
    ret = mbedtls_ssl_read( &sslctx, rcvbuf.ucptr(), rcvbuf.length());
    if (ret > 0)
    {
        rcvbuf.resize(ret);
        msg.Clear();
        if (msg.ParseFromString(rcvbuf) == true)
            return GOT_MESSAGE;
        else
        {
            std::cout << "message parsing failed\n";
        }
    }
    if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY || ret == 0)
    {
        mbedtls_ssl_close_notify( &sslctx );
        send_close_notify = false;
        mbedtls_net_free(&netctx);
        if (ret == 0)
            std::cout << "NOTE: Remote Disconnect Unclean\n";
        mbedtls_ssl_session_reset( &sslctx );
        return GOT_DISCONNECT;
    }
    printf("mbedtls_ssl_read returned %d\n", ret);
    return READ_MORE;
}

// returns true if ok, false if not
bool
ProtoSSLConnClient :: send_message(const MESSAGE &msg)
{
    int ret;
    WaitUtil::Lock lock(&ssl_lock);

    if (msg.SerializeToString(&outbuf) == false)
    {
        std::cout << "_sendMessage failed to serialize\n";
        // error?
        return false;
    }

    do {
        ret = mbedtls_ssl_write( &sslctx, outbuf.ucptr(), outbuf.length() );
    } while (ret == MBEDTLS_ERR_SSL_WANT_READ ||
             ret == MBEDTLS_ERR_SSL_WANT_WRITE);

    if (ret < 0)
    {
        char strbuf[200];
        mbedtls_strerror(ret,strbuf,sizeof(strbuf));
        std::cout << "ssl_write returned " << std::hex << ret << ": "
                  << strbuf << std::endl;
    }

    if (ret == MBEDTLS_ERR_NET_CONN_RESET)
    {
        std::cout << "ssl_write peer closed connection" << std::endl;
        return false;
    }

    return true;
}
