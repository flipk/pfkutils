
#include "libprotossl.h"
#include "bufprintf.h"
#include <sstream>
#include <mbedtls/oid.h>

using namespace ProtoSSL;

ProtoSSLConnClient :: ProtoSSLConnClient(ProtoSSLMsgs * _msgs,
                                         mbedtls_net_context new_netctx,
                                         bool _use_tcp,
                                         unsigned char *client_ip,
                                         size_t cliip_len)
    : msgs(_msgs), use_tcp(_use_tcp)
{
    _ok = false;
    send_close_notify = false;
    ssl_initialized = false;
    netctx = new_netctx;
    net_initialized = true;
    WaitUtil::Lock lock(&ssl_lock);
    _ok = init_common(client_ip, cliip_len);
    msgs->registerClient(this);
}

ProtoSSLConnClient :: ProtoSSLConnClient(ProtoSSLMsgs * _msgs,
                                         const std::string &remoteHost,
                                         int remotePort,
                                         bool _use_tcp)
    : msgs(_msgs), use_tcp(_use_tcp)
{
    _ok = false;
    send_close_notify = false;
    ssl_initialized = false;
    net_initialized = false;

    WaitUtil::Lock lock(&ssl_lock);
    msgs->registerClient(this);
    Bufprintf<20> portString;
    portString.print("%d", remotePort);
    mbedtls_net_init(&netctx);
    int ret = mbedtls_net_connect(&netctx, remoteHost.c_str(),
                                  portString.getBuf(),
                                  use_tcp ?
                                  MBEDTLS_NET_PROTO_TCP :
                                  MBEDTLS_NET_PROTO_UDP );
    if (ret != 0)
    {
        char strbuf[200];
        mbedtls_strerror( ret, strbuf, sizeof(strbuf));
        fprintf(stderr, "net bind returned 0x%x: %s\n", -ret, strbuf);
        return;
    }

    // net_connect has a bug -- if connection fails, it closes
    // netctx.fd but does not set it to -1. then if you net_free,
    // you end up double-closing the fd (leading to a possible race
    // with other threads reusing that fd#).
    net_initialized = true;
    _ok = init_common(NULL, 0);
}

// this function requires ssl_lock to be locked.
bool
ProtoSSLConnClient :: init_common(unsigned char *client_ip, size_t cliip_len)
{
    int ret;

    mbedtls_ssl_init( &sslctx );
    ssl_initialized = true;
    mbedtls_ssl_setup( &sslctx, &msgs->sslcfg );
    mbedtls_ssl_set_hs_authmode( &sslctx, MBEDTLS_SSL_VERIFY_REQUIRED );
    mbedtls_ssl_set_bio( &sslctx, &netctx,
                 &mbedtls_net_send, &mbedtls_net_recv,
                 &mbedtls_net_recv_timeout);

    if (use_tcp == false && client_ip != NULL)
    {
        int ret;
        /* For HelloVerifyRequest cookies */
        if( ( ret = mbedtls_ssl_set_client_transport_id( &sslctx,
                                                         client_ip, cliip_len ) ) != 0 )
        {
            fprintf(stderr, "ProtoSSLConnClient :: init_common : ERROR: "
                    "mbedtls_ssl_set_client_transport_id() returned -0x%x\n\n", -ret );
        }
    }

    mbedtls_ssl_set_timer_cb( &sslctx, &timer,
                              mbedtls_timing_set_delay,
                              mbedtls_timing_get_delay );

    while ((ret = mbedtls_ssl_handshake( &sslctx )) != 0)
    {
        // note in DTLS, MBEDTLS_ERR_SSL_HELLO_VERIFY_REQUIRED is a common
        // response; the documentation says it is "normal for this to fail
        // half the time." what ever that means. the client silently retries,
        // and we get into a new client object just fine, so this is ok.
        if (ret == MBEDTLS_ERR_SSL_HELLO_VERIFY_REQUIRED)
        {
// not printing this, because it's useless information.
//    fprintf(stderr,
//            "ProtoSSLConnClient: HELLO_VERIFY_REQUIRED (normal), retrying\n");
            mbedtls_ssl_session_reset( &sslctx );
            return false;
        }

        if (ret != MBEDTLS_ERR_SSL_WANT_READ &&
            ret != MBEDTLS_ERR_SSL_WANT_WRITE )
        {
            char strbuf[200];
            mbedtls_strerror( ret, strbuf, sizeof(strbuf));
            fprintf(stderr, " failed\n  ! ssl_handshake returned 0x%x: %s\n\n",
                    -ret, strbuf );
            return false;
        }
    }

    if( ( ret = mbedtls_ssl_get_verify_result( &sslctx ) ) != 0 )
    {
        fprintf(stderr, "ssl_get_verify_result failed (ret = 0x%x)\n", ret );

        char vrfy_buf[512];
        mbedtls_x509_crt_verify_info( vrfy_buf, sizeof( vrfy_buf ), "  ! ", ret );
        fprintf(stderr, "%s\n", vrfy_buf);
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
    if (net_initialized)
        mbedtls_net_free(&netctx);
    if (ssl_initialized)
        mbedtls_ssl_free(&sslctx);
}

#if 0
static std::string
print_asn1buf(const mbedtls_asn1_buf &b, bool is_str)
{
    std::ostringstream o;

    pxfe_string  p((const char*) b.p, b.len);

    o << "tag " << b.tag << " "
      << "len " << b.len << " "
      << "p ";
    if (is_str)
        o << p;
    else
        o << p.format_hex();

    return o.str();
}
/*
oid : tag 6 len 3 p 550403
     tag = MBEDTLS_ASN1_OID
     p = MBEDTLS_OID_ISO_CCITT_DS
         MBEDTLS_OID_AT
         MBEDTLS_OID_AT_CN

val : tag 12 len 14 p "ubuntu test i3"
     tag = MBEDTLS_ASN1_UTF8_STRING

oid : tag 6 len 9 p 2a864886f70d010901
     tag = MBEDTLS_ASN1_OID
     p = MBEDTLS_OID_ISO_MEMBER_BODIES
         MBEDTLS_OID_COUNTRY_US
         MBEDTLS_OID_ORG_RSA_DATA_SECURITY
         MBEDTLS_OID_PKCS
         MBEDTLS_OID_PKCS9
         MBEDTLS_OID_PKCS9_EMAIL

val : tag 22 len 14 p "client@pfk.org"
     tag = MBEDTLS_ASN1_IA5_STRING

 */
#endif

bool
ProtoSSLConnClient :: get_peer_info(ProtoSSLPeerInfo &info)
{
    const mbedtls_x509_crt *crt = mbedtls_ssl_get_peer_cert( &sslctx );

    if (crt == NULL)
        return false;

    const std::string  cn(MBEDTLS_OID_AT_CN);
    const std::string  em(MBEDTLS_OID_PKCS9_EMAIL);
    const std::string  ou(MBEDTLS_OID_AT_ORG_UNIT);

    struct sockaddr_in sa;
    socklen_t  salen = sizeof(sa);

    if (getpeername(get_fd(), (struct sockaddr *)&sa, &salen) < 0)
    {
        int e = errno;
        char * err = strerror(errno);
        std::cerr << "getpeername failed: " << e << ": " << err << std::endl;
    }
    else
    {
        uint32_t addr = ntohl(sa.sin_addr.s_addr);
        std::ostringstream s;
        s << (int) ((addr >> 24) & 0xFF) << "."
          << (int) ((addr >> 16) & 0xFF) << "."
          << (int) ((addr >>  8) & 0xFF) << "."
          << (int) ((addr >>  0) & 0xFF);
        info.ipaddr = s.str();
    }

    const mbedtls_asn1_named_data * d = &crt->subject;
    while (d)
    {
        std::string oid((const char *)d->oid.p, d->oid.len);
        std::string val((const char *)d->val.p, d->val.len);

        if (oid == cn)
            info.common_name = val;
        else if (oid == em)
            info.pkcs9_email = val;
        else if (oid == ou)
            info.org_unit = val;

        d = d->next;
    }

    return true;
}

ProtoSSLConnClient :: read_return_t
ProtoSSLConnClient :: handle_read_raw(std::string &buffer)
{
    int ret;
    WaitUtil::Lock lock(&ssl_lock);

    buffer.resize(PROTOSSL_MAX_CONTENT_LEN);
    ret = mbedtls_ssl_read( &sslctx,
                            (unsigned char*) buffer.c_str(),
                            buffer.length());
    if (ret > 0)
    {
        buffer.resize(ret);
        return GOT_MESSAGE;
    }
    if (ret == MBEDTLS_ERR_SSL_TIMEOUT)
    {
        return GOT_TIMEOUT;
    }
    if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY ||
        ret == MBEDTLS_ERR_NET_RECV_FAILED       ||
        ret == 0)
    {
        mbedtls_ssl_close_notify( &sslctx );
        send_close_notify = false;
        mbedtls_net_free(&netctx);
        if (ret == 0)
            std::cerr << "NOTE: Remote Disconnect Unclean\n";
        mbedtls_ssl_session_reset( &sslctx );
        return GOT_DISCONNECT;
    }
    fprintf(stderr, "mbedtls_ssl_read returned %d\n", ret);
    return READ_MORE;
}

ProtoSSLConnClient :: read_return_t
ProtoSSLConnClient :: handle_read(MESSAGE &msg)
{
    read_return_t rr = handle_read_raw(rcvbuf);

    if (rr == GOT_MESSAGE)
    {
        msg.Clear();
        if (msg.ParseFromString(rcvbuf) == true)
            return GOT_MESSAGE;
        // else
        std::cerr << "message parsing failed\n";
        return GOT_DISCONNECT;
    }

    return rr;
}

// returns true if ok, false if not
bool
ProtoSSLConnClient :: send_raw(const std::string &buffer)
{
    int ret;
    WaitUtil::Lock lock(&ssl_lock);

    do {
        ret = mbedtls_ssl_write( &sslctx,
                                 (unsigned char *)buffer.c_str(),
                                 buffer.length() );
    } while (ret == MBEDTLS_ERR_SSL_WANT_READ ||
             ret == MBEDTLS_ERR_SSL_WANT_WRITE);

    if (ret < 0)
    {
        char strbuf[200];
        mbedtls_strerror(ret,strbuf,sizeof(strbuf));
        std::cerr << "ssl_write returned " << std::hex << ret << ": "
                  << strbuf << std::endl;
    }

    if (ret == MBEDTLS_ERR_NET_CONN_RESET)
    {
        std::cerr << "ssl_write peer closed connection" << std::endl;
        return false;
    }

    return true;
}

bool
ProtoSSLConnClient :: send_message(const MESSAGE &msg)
{
    if (msg.SerializeToString(&outbuf) == false)
    {
        std::cerr << "_sendMessage failed to serialize\n";
        // error?
        return false;
    }

    return send_raw(outbuf);
}
