
#include "libprotossl.h"

#include <sys/select.h>
#include <iostream>

namespace ProtoSSL {

__ProtoSSLMsgs::__ProtoSSLMsgs(const ProtoSSLCertParams &params,
                               int listeningPort)
{
    int ret;
    port = listeningPort;
    isServer = true;
    good = true;

    if (initCommon() == false)
    {
        std::cout << "initCommon failed\n";
        good = false;
    }

    if (good)
        if (loadCertificates(params) == false)
        {
            std::cout << "loadCertificates failed\n";
            good = false;
        }

    state = WAIT_FOR_CONN;

    if (good)
        if ((ret = net_bind(&listen_fd, NULL, port)) != 0)
        {
            printf(" net_bind failed error -0x%x\n", -ret);
            good = false;
        }
}

__ProtoSSLMsgs::__ProtoSSLMsgs(const ProtoSSLCertParams &params,
                               const std::string &_remoteHost,
                               int remotePort)
{
    remoteHost = _remoteHost;
    port = remotePort;
    isServer = false;
    good = true;

    if (initCommon() == false)
    {
        std::cout << "initCommon failed\n";
        good = false;
    }

    if (good)
        if (loadCertificates(params) == false)
        {
            std::cout << "loadCertificates failed\n";
            good = false;
        }

    state = TRY_CONN;

    if (good)
        ssl_set_hostname( &sslctx, otherCommonName.c_str() );
}

bool
__ProtoSSLMsgs::initCommon(void)
{
    int ret;
    static const char * pers = "ProtoSSLDRBG";

    entropy_init( &entropy );

    if( ( ret = ctr_drbg_init( &ctr_drbg, entropy_func, &entropy,
                               (const unsigned char *) pers,
                               strlen( pers ) ) ) != 0 )
    {
        printf( " ctr_drbg_init returned -0x%x\n", -ret );
        return false;
    }

    memset( &mycert, 0, sizeof( x509_cert ) );
    memset( &cacert, 0, sizeof( x509_cert ) );
    rsa_init( &mykey, RSA_PKCS_V15, 0 );

    if ((ret = ssl_init( &sslctx )) != 0)
    {
        printf("ssl init returned -0x%x\n", -ret);
        return false;
    }

    ssl_set_authmode( &sslctx, SSL_VERIFY_REQUIRED );
    ssl_set_rng( &sslctx, ctr_drbg_random, &ctr_drbg );

    if (isServer)
        ssl_set_endpoint( &sslctx, SSL_IS_SERVER );
    else
        ssl_set_endpoint( &sslctx, SSL_IS_CLIENT );

    fd = -1;
    listen_fd = -1;

    return true;
}

//virtual
__ProtoSSLMsgs::~__ProtoSSLMsgs(void)
{
    if (fd != -1)
        net_close(fd);
    if (listen_fd != -1)
        net_close(listen_fd);

    ssl_free( &sslctx );
    rsa_free( &mykey );
    x509_free( &mycert );
    x509_free( &cacert );
}

//private
bool __ProtoSSLMsgs::loadCertificates(const ProtoSSLCertParams &params)
{
    int ret;

    otherCommonName = params.otherCommonName;

    if ((ret = x509parse_crtfile( &cacert, params.caCertFile.c_str())) != 0)
    {
        printf( " 1 x509parse_crt returned -0x%x\n\n", -ret );
        return false;
    }

    if ((ret = x509parse_crtfile( &mycert, params.myCertFile.c_str() )) != 0)
    {
        printf( " 2 x509parse_crt returned -0x%x\n\n", -ret );
        return false;
    }

    if ((ret = x509parse_keyfile(
             &mykey, params.myKeyFile.c_str(),
             (params.myKeyPassword == "") ? NULL :
             params.myKeyPassword.c_str() )) != 0)
    {
        printf( " 3 x509parse_key returned -0x%x\n\n", -ret );
        return false;
    }

    // this string below is the srv.crt Common Name field
    ssl_set_ca_chain( &sslctx, &cacert, NULL, otherCommonName.c_str());
    ssl_set_own_cert( &sslctx, &mycert, &mykey );

    return true;
}

bool
__ProtoSSLMsgs::handShakeCommon(void)
{
    int ret;

    while ((ret = ssl_handshake( &sslctx )) != 0)
    {
        if (ret != POLARSSL_ERR_NET_WANT_READ &&
            ret != POLARSSL_ERR_NET_WANT_WRITE )
        {
            printf( " failed\n  ! ssl_handshake returned -0x%x\n\n", -ret );
            return false;
        }
    }

    if( ( ret = ssl_get_verify_result( &sslctx ) ) != 0 )
    {
        printf( "ssl_get_verify_result failed\n" );
        if( ( ret & BADCERT_EXPIRED ) != 0 )
            printf( "  ! server certificate has expired\n" );
        if( ( ret & BADCERT_REVOKED ) != 0 )
            printf( "  ! server certificate has been revoked\n" );
        if( ( ret & BADCERT_CN_MISMATCH ) != 0 )
            printf( "  ! CN mismatch (expected CN=%s)\n",
                    "PolarSSL Server 1" );
        if( ( ret & BADCERT_NOT_TRUSTED ) != 0 )
            printf( "  ! self-signed or not signed by a trusted CA\n" );
        printf( "\n" );
        return false;
    }

    return true;
}

//protected
bool
__ProtoSSLMsgs::_sendMessage(const google::protobuf::Message& msg)
{
    std::string outBuf;
    int ret;

    if (!good)
    {
        std::cout << "_sendMessage: good is false\n";
        return false;
    }

    if (msg.SerializeToString(&outBuf) == false)
    {
        std::cout << "_sendMessage failed to serialize\n";
        // error?
        return false;
    }

    do {
        ret = ssl_write( &sslctx, reinterpret_cast<const unsigned char *>(
                             outBuf.c_str()), outBuf.length() );
    } while (ret == POLARSSL_ERR_NET_WANT_READ ||
             ret == POLARSSL_ERR_NET_WANT_WRITE);

    if (ret < 0)
        std::cout << "ssl_write returned " << std::hex << -ret << std::endl;

    if (ret == POLARSSL_ERR_NET_CONN_RESET)
    {
        std::cout << "ssl_write peer closed connection" << std::endl;
        return false;
    }

    return true;
}

//protected
ProtoSSLEventType
__ProtoSSLMsgs::_getEvent(int &connectionId, int timeoutMs)
{
    int ret;
    struct timeval tv;
    fd_set rfds;
    tv.tv_sec = timeoutMs / 1000;
    tv.tv_usec = (timeoutMs % 1000) * 1000;
    FD_ZERO(&rfds);
    if (!good)
    {
        std::cout << "_getEvent: good is false\n";
        return PROTOSSL_RETRY;
    }
    switch (state)
    {
    case WAIT_FOR_CONN:
        FD_SET(listen_fd, &rfds);
        if (select(listen_fd+1,&rfds,NULL,NULL,&tv) <= 0)
            return PROTOSSL_TIMEOUT;
        if ((ret = net_accept(listen_fd, &fd, NULL)) != 0)
        {
            printf(" net_accept failed error -0x%x\n", -ret);
            return PROTOSSL_RETRY;
        }
        ssl_set_bio( &sslctx, net_recv, &fd, net_send, &fd );
        if (handShakeCommon() == false)
            return PROTOSSL_RETRY;
        state = SERVER_CONNECTED;
        return PROTOSSL_CONNECT;

    case TRY_CONN:
        if ((ret = net_connect(&fd, remoteHost.c_str(), port)) != 0)
        {
            printf("net_connect failed with error -0x%x\n", -ret);
            return PROTOSSL_RETRY;
        }
        ssl_set_bio( &sslctx, net_recv, &fd, net_send, &fd );
        if (handShakeCommon() == false)
            return PROTOSSL_RETRY;
        state = CLIENT_CONNECTED;
        return PROTOSSL_CONNECT;

    case SERVER_CONNECTED:
    case CLIENT_CONNECTED:
        FD_SET(fd, &rfds);
        if (select(fd+1, &rfds, NULL, NULL, &tv) <= 0)
            return PROTOSSL_TIMEOUT;
        rcvbuf.resize(SSL_MAX_CONTENT_LEN);
        ret = ssl_read( &sslctx,
                            (unsigned char*) rcvbuf.c_str(),
                            SSL_MAX_CONTENT_LEN );
        if (ret > 0)
        {
            rcvbuf.resize(ret);
            rcvdMsg->Clear();
            if (rcvdMsg->ParseFromString(rcvbuf) == true)
                return PROTOSSL_MESSAGE;
            std::cout << "message parsing failed\n";
            return PROTOSSL_RETRY;
        }
        if (ret == POLARSSL_ERR_NET_WANT_READ ||
            ret == POLARSSL_ERR_NET_WANT_WRITE)
            return PROTOSSL_RETRY;
        if (ret == POLARSSL_ERR_SSL_PEER_CLOSE_NOTIFY ||
            ret == 0)
        {
            net_close(fd);
            fd = -1;
            if (state == SERVER_CONNECTED)
                state = WAIT_FOR_CONN;
            else
                state = TRY_CONN;
            if (ret == 0)
                std::cout << "remote disconnect unclean\n";
            ssl_session_reset( &sslctx );
            return PROTOSSL_DISCONNECT;
        }
        std::cout << "ssl read returns " << ret << std::endl;
        return PROTOSSL_RETRY;

    default:
        std::cerr << "unknown state " << state << std::endl;
        ;
    }
    return PROTOSSL_RETRY;
}

}; // namespace ProtoSSL
