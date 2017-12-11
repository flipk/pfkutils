/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */
/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/

#include "libprotossl.h"
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <iostream>

#define ssl_personality "ProtoSSLDRBG"
#if POLARSSL

#define MBEDTLS_ERR_SSL_WANT_READ POLARSSL_ERR_NET_WANT_READ
#define MBEDTLS_ERR_SSL_WANT_WRITE POLARSSL_ERR_NET_WANT_WRITE
#define MBEDTLS_X509_BADCERT_EXPIRED BADCERT_EXPIRED
#define MBEDTLS_X509_BADCERT_REVOKED BADCERT_REVOKED
#define MBEDTLS_X509_BADCERT_CN_MISMATCH BADCERT_CN_MISMATCH
#define MBEDTLS_X509_BADCERT_NOT_TRUSTED BADCERT_NOT_TRUSTED
#define MBEDTLS_SSL_MAX_CONTENT_LEN SSL_MAX_CONTENT_LEN
#define MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY POLARSSL_ERR_SSL_PEER_CLOSE_NOTIFY
#define MBEDTLS_ERR_SSL_WANT_READ POLARSSL_ERR_NET_WANT_READ
#define MBEDTLS_ERR_SSL_WANT_WRITE POLARSSL_ERR_NET_WANT_WRITE
#define MBEDTLS_ERR_NET_CONN_RESET POLARSSL_ERR_NET_CONN_RESET

#define mbedtls_ssl_handshake ssl_handshake
#define mbedtls_ssl_get_verify_result ssl_get_verify_result
#define mbedtls_ssl_read ssl_read
#define mbedtls_ssl_session_reset ssl_session_reset
#define mbedtls_ssl_close_notify ssl_close_notify
#define mbedtls_ssl_write ssl_write
#define mbedtls_entropy_init entropy_init
#define mbedtls_x509_crt x509_crt
#define mbedtls_pk_init pk_init
#define mbedtls_pk_free pk_free
#define mbedtls_x509_crt_free x509_crt_free
#define mbedtls_x509_crt_parse_file x509_crt_parse_file
#define mbedtls_x509_crt_parse x509_crt_parse
#define mbedtls_pk_parse_keyfile pk_parse_keyfile
#define mbedtls_pk_parse_key pk_parse_key
#define mbedtls_strerror polarssl_strerror

#endif

namespace ProtoSSL {

//
// ProtoSSLCertParams
//

ProtoSSLCertParams::ProtoSSLCertParams(
    const std::string &_caCert, // file:/...
    const std::string &_myCert, // file:/...
    const std::string &_myKey,  // file:/...
    const std::string &_myKeyPassword,
    const std::string &_otherCommonName)
    : caCert(_caCert), myCert(_myCert), myKey(_myKey),
      myKeyPassword(_myKeyPassword), otherCommonName(_otherCommonName)
{
}

ProtoSSLCertParams::~ProtoSSLCertParams(void)
{
}

//
// _ProtoSSLConn
//

_ProtoSSLConn::_ProtoSSLConn(MESSAGE &_rcvdMessage)
#if POLARSSL
    : fd(-1),
#else
    : netctx_initialized(false),
#endif
      rcvdMessage(_rcvdMessage), msgs(NULL)
{
    thread_running = false;
    exitPipe[0] = exitPipe[1] = -1;
}

//virtual
_ProtoSSLConn::~_ProtoSSLConn(void)
{
    if (exitPipe[0] != -1)
    {
        close(exitPipe[0]);
        close(exitPipe[1]);
    }
#if POLARSSL
    if (fd > 0)
    {
        net_close(fd);
        msgs->deregisterConn(fd,this);
        ssl_free(&sslctx);
    }
#else
    if (netctx_initialized)
    {
        msgs->deregisterConn(netctx.fd,this);
        mbedtls_net_free(&netctx);
        mbedtls_ssl_free(&sslctx);
    }
#endif
}

#if POLARSSL
bool
_ProtoSSLConn::_startThread(ProtoSSLMsgs * _msgs, bool isServer, int _fd)
{
    int ret;
    char strbuf[200];

    fd = _fd;
    msgs = _msgs;

    if ((ret = ssl_init( &sslctx )) != 0)
    {
        mbedtls_strerror(ret, strbuf, sizeof(strbuf));
        printf("ssl init returned 0x%x: %s\n", -ret, strbuf);
        return false;
    }

    ssl_set_authmode( &sslctx, SSL_VERIFY_REQUIRED );
    ssl_set_rng( &sslctx, ctr_drbg_random, &msgs->ctr_drbg );

    if (isServer)
        ssl_set_endpoint( &sslctx, SSL_IS_SERVER );
    else
        ssl_set_endpoint( &sslctx, SSL_IS_CLIENT );

    // this string below is the srv.crt Common Name field
    ssl_set_ca_chain( &sslctx, &msgs->cacert,
                      NULL, msgs->otherCommonName.c_str());
    ssl_set_own_cert( &sslctx, &msgs->mycert, &msgs->mykey );

    ssl_set_bio( &sslctx, net_recv, &fd, net_send, &fd );

    if (pipe(exitPipe) < 0)
        printf("_ProtoSSLConn::_startThread: "
               "pipe error %d: %s\n", errno, strerror(errno));

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&thread_id, &attr,
                   &_ProtoSSLConn::threadMain, (void*) this);
    pthread_attr_destroy(&attr);

    return true;
}
#else
bool
_ProtoSSLConn::_startThread(ProtoSSLMsgs * _msgs, bool isServer,
                            const mbedtls_net_context &_netctx)
{
    msgs = _msgs;
    netctx = _netctx;
    netctx_initialized = true;

    mbedtls_ssl_init( &sslctx );
    mbedtls_ssl_setup( &sslctx, &msgs->sslcfg );

    mbedtls_ssl_set_hs_authmode( &sslctx, MBEDTLS_SSL_VERIFY_REQUIRED );
    mbedtls_ssl_set_bio( &sslctx, &netctx,
                 &mbedtls_net_send, &mbedtls_net_recv,
                 &mbedtls_net_recv_timeout);

    if (pipe(exitPipe) < 0)
        printf("_ProtoSSLConn::_startThread: "
               "pipe error %d: %s\n", errno, strerror(errno));

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&thread_id, &attr,
                   &_ProtoSSLConn::threadMain, (void*) this);
    pthread_attr_destroy(&attr);

    return true;
}
#endif

//static
void *
_ProtoSSLConn::threadMain(void *arg)
{
    _ProtoSSLConn * conn = (_ProtoSSLConn *) arg;
    conn->thread_running = true;
    conn->_threadMain();
    conn->thread_running = false;
    delete conn;
    return NULL;
}

void
_ProtoSSLConn::_threadMain(void)
{
    int ret;
    int maxfd;
    fd_set rfds, rfds_proto;
    bool done = false;
    bool send_close_notify = false;

#if !POLARSSL
    int fd = netctx.fd;
#endif

    while ((ret = mbedtls_ssl_handshake( &sslctx )) != 0)
    {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ &&
            ret != MBEDTLS_ERR_SSL_WANT_WRITE )
        {
            char strbuf[200];
            mbedtls_strerror( ret, strbuf, sizeof(strbuf));
            printf( " failed\n  ! ssl_handshake returned 0x%x: %s\n\n",
                    -ret, strbuf );
            goto bail;
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
        goto bail;
    }

    handleConnect();

    FD_ZERO(&rfds_proto);
    maxfd = fd;
    FD_SET(fd, &rfds_proto);
    if (fd < exitPipe[0])
        maxfd = exitPipe[0];
    FD_SET(exitPipe[0], &rfds_proto);

    while (!done)
    {
        rfds = rfds_proto;
        select(maxfd, &rfds, NULL, NULL, NULL);
        if (FD_ISSET(fd, &rfds))
        {
            rcvbuf.resize(MBEDTLS_SSL_MAX_CONTENT_LEN);
            ret = mbedtls_ssl_read( &sslctx,
                            (unsigned char*) rcvbuf.c_str(),
                            MBEDTLS_SSL_MAX_CONTENT_LEN );
            if (ret > 0)
            {
                rcvbuf.resize(ret);
                rcvdMessage.Clear();
                if (rcvdMessage.ParseFromString(rcvbuf) == true)
                {
                    if (_messageHandler() == false)
                    {
                        send_close_notify = true;
                        done = true;
                    }
                }
                else
                {
                    std::cout << "message parsing failed\n";
                    send_close_notify = true;
                    done = true;
                }
            }
            if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY || ret == 0)
            {
#if POLARSSL
                net_close(fd);
                fd = -1;
#else
                mbedtls_net_free(&netctx);
                netctx_initialized = false;
#endif
                if (ret == 0)
                    std::cout << "remote disconnect unclean\n";
                mbedtls_ssl_session_reset( &sslctx );
                done = true;
            }
        }
        if (FD_ISSET(exitPipe[0], &rfds))
        {
            done = true;
        }
    }

bail:
    if (send_close_notify)
        mbedtls_ssl_close_notify( &sslctx );
}

bool
_ProtoSSLConn::_sendMessage(MESSAGE &msg)
{
    int ret;
    WaitUtil::Lock   lck(&fdLock);

    if (msg.SerializeToString(&outbuf) == false)
    {
        std::cout << "_sendMessage failed to serialize\n";
        // error?
        return false;
    }

    do {
        ret = mbedtls_ssl_write( &sslctx,
                                 reinterpret_cast<const unsigned char *>(
                                     outbuf.c_str()), outbuf.length() );
    } while (ret == MBEDTLS_ERR_SSL_WANT_READ ||
             ret == MBEDTLS_ERR_SSL_WANT_WRITE);

    msg.Clear();
    outbuf.clear();

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

void
_ProtoSSLConn::closeConnection(void)
{
    char dummy = 1;
    if (::write(exitPipe[1], &dummy, 1) < 0)
        printf("_ProtoSSLConn::closeConnection: "
               "write to exit pipe: %d: %s\n", errno, strerror(errno));
}

void
_ProtoSSLConn::stopMsgs(void)
{
    msgs->stop();
}

//
// ProtoSSLMsgs
//

ProtoSSLMsgs::ProtoSSLMsgs(void)
{
    mbedtls_entropy_init( &entropy );
    memset( &mycert, 0, sizeof( mbedtls_x509_crt ) );
    memset( &cacert, 0, sizeof( mbedtls_x509_crt ) );
    mbedtls_pk_init( &mykey ); // rsa_init( &mykey, RSA_PKCS_V15, 0 );

    if (pipe(exitPipe) < 0)
        printf("ProtoSSLMsgs::ProtoSSLMsgs: "
               "pipe failed: %d: %s\n", errno, strerror(errno));
#if !POLARSSL
    mbedtls_ssl_config_init( &sslcfg );
    mbedtls_ssl_config_defaults( &sslcfg, MBEDTLS_SSL_IS_SERVER,
                                 MBEDTLS_SSL_TRANSPORT_STREAM,
                                 MBEDTLS_SSL_PRESET_DEFAULT );
    //mbedtls_ssl_conf_dbg( &sslcfg, f_dbg, p_dbg );
    //mbedtls_ssl_conf_read_timeout( &sslcfg, 0 );

    mbedtls_ctr_drbg_init( &ctr_drbg );
    mbedtls_ctr_drbg_seed( &ctr_drbg, mbedtls_entropy_func, &entropy,
                           (const unsigned char *) ssl_personality,
                           strlen( ssl_personality ) );
    mbedtls_ssl_conf_rng( &sslcfg, &mbedtls_ctr_drbg_random, &ctr_drbg );

    mbedtls_ssl_conf_authmode( &sslcfg,
                               MBEDTLS_SSL_VERIFY_REQUIRED );

    // doesn't appear to be needed?
    //mbedtls_ssl_conf_verify( &sslcfg, f_vrfy, p_vrfy )
#endif
}

ProtoSSLMsgs::~ProtoSSLMsgs(void)
{
    char dummy = 1;
    {
        WaitUtil::Lock  lck(&connLock);
        connMap::iterator  cit;
        // tell all connection threads to exit
        for (cit = conns.begin(); cit != conns.end(); cit++)
        {
            _ProtoSSLConn * conn = cit->second;
            conn->closeConnection();
        }
        serverInfoMap::iterator sit;
        // tell all server threads to exit
        for (sit = servers.begin(); sit != servers.end(); sit++)
        {
            serverInfo &si = sit->second;
            if (::write(si.exitPipe[1], &dummy, 1) < 0)
                printf("ProtoSSLMsgs::~ProtoSSLMsgs: "
                       "write exit pipe: %d: %s\n", errno, strerror(errno));
        }
    } // unlock

    // wait for all connection threads and server threads
    // to exit, call in, and remove them selves from the
    // maps as they exit.
    while ((conns.size() > 0) || (servers.size() > 0))
        usleep(1);

    // TODO pop servers list and clean
    // TODO pop connMap and clean
    mbedtls_pk_free(&mykey);
    mbedtls_x509_crt_free(&mycert);
    mbedtls_x509_crt_free(&cacert);

#if !POLARSSL
    mbedtls_ssl_config_free(&sslcfg);
    mbedtls_entropy_free(&entropy);
    mbedtls_ctr_drbg_free(&ctr_drbg);
#endif

    close(exitPipe[0]);
    close(exitPipe[1]);
}

bool
ProtoSSLMsgs::loadCertificates(const ProtoSSLCertParams &params)
{
    int ret;
    char strbuf[200];

#if POLARSSL
    if( ( ret = ctr_drbg_init( &ctr_drbg, entropy_func, &entropy,
                               (const unsigned char *) ssl_personality,
                               strlen( ssl_personality ) ) ) != 0 )
    {
        mbedtls_strerror( ret, strbuf, sizeof(strbuf));
        printf( " ctr_drbg_init returned 0x%x: %s\n", -ret, strbuf );
        return false;
    }
#endif

    otherCommonName = params.otherCommonName;

    if (params.caCert.compare(0,5,"file:") == 0)
        ret = mbedtls_x509_crt_parse_file( &cacert,
                                   params.caCert.c_str() + 5);
    else
        ret = mbedtls_x509_crt_parse( &cacert,
                              (const unsigned char *) params.caCert.c_str(),
                              params.caCert.size());
    if (ret != 0)
    {
        mbedtls_strerror( ret, strbuf, sizeof(strbuf));
        printf( " 1 x509parse_crt returned 0x%x: %s\n\n", -ret, strbuf );
        return false;
    }

    if (params.myCert.compare(0,5,"file:") == 0)
        ret = mbedtls_x509_crt_parse_file( &mycert,
                                   params.myCert.c_str()+5);
    else
        ret = mbedtls_x509_crt_parse( &mycert,
                              (const unsigned char *) params.myCert.c_str(),
                              params.myCert.size());
    if (ret != 0)
    {
        mbedtls_strerror( ret, strbuf, sizeof(strbuf));
        printf( " 2 x509parse_crt returned 0x%x: %s\n\n", -ret, strbuf );
        return false;
    }

    const char *keyPassword = NULL;
    int keyPasswordLen = 0;
    if (params.myKeyPassword != "")
    {
        keyPassword = params.myKeyPassword.c_str();
        keyPasswordLen = params.myKeyPassword.size();
    }

    if (params.myKey.compare(0,5,"file:") == 0)
        ret = mbedtls_pk_parse_keyfile( &mykey, 
                                params.myKey.c_str() + 5,
                                keyPassword);
    else
        ret = mbedtls_pk_parse_key( &mykey, 
                            (const unsigned char *) params.myKey.c_str(),
                            params.myKey.size(),
                            (const unsigned char *) keyPassword,
                            keyPasswordLen);
    if (ret != 0)
    {
        mbedtls_strerror( ret, strbuf, sizeof(strbuf));
        printf( " 3 pk_parse_keyfile returned 0x%x: %s\n\n", -ret, strbuf );
        return false;
    }

#if !POLARSSL
    mbedtls_ssl_conf_ca_chain( &sslcfg, &cacert, NULL );
    mbedtls_ssl_conf_own_cert( &sslcfg, &mycert, &mykey );
#endif

    return true;
}

void
ProtoSSLMsgs::deregisterConn(int fd,_ProtoSSLConn *conn)
{
    WaitUtil::Lock  lck(&connLock);
    connMap::iterator it = conns.find(fd);
    if (it != conns.end())
    {
        conns.erase(it);
    }
}

bool
ProtoSSLMsgs::startServer(ProtoSSLConnFactory &factory,
                          int listeningPort)
{
    int ret, fd = -1;
    char strbuf[200];

#if !POLARSSL
    mbedtls_net_context netctx;
    // note server flag already set in sslcfg
#endif

#if POLARSSL
    ret = net_bind(&fd, NULL, listeningPort);
#else
    mbedtls_net_init( &netctx );
    char portString[8];
    sprintf(portString,"%d",listeningPort);
    ret = mbedtls_net_bind(&netctx, NULL, portString, MBEDTLS_NET_PROTO_TCP);
#endif
    if (ret != 0)
    {
        mbedtls_strerror( ret, strbuf, sizeof(strbuf));
        printf("net bind returned 0x%x: %s\n", -ret, strbuf);
        return false;
    }

    WaitUtil::Lock lck(&connLock);
    serverInfo & si = servers[fd]; // note this creates new entry

#if POLARSSL
    si.fd = fd;
#else
    si.netctx = netctx;
#endif
    si.msgs = this;
    if (pipe(si.exitPipe) < 0)
        printf("ProtoSSLMsgs::startServer: "
               "pipe failed: %d: %s\n", errno, strerror(errno));
    si.factory = &factory;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&si.thread_id, &attr,
                   &ProtoSSLMsgs::serverThread, (void*) &si);
    pthread_attr_destroy(&attr);

    return true;
}

//static
void *
ProtoSSLMsgs::serverThread(void * arg)
{
    serverInfo * si = (serverInfo *) arg;
    ProtoSSLMsgs * obj = si->msgs;
    obj->_serverThread(si);
    delete si;
    return NULL;
}

void
ProtoSSLMsgs::_serverThread(serverInfo * si)
{
    fd_set  rfds, rfds_proto;
    int ret, fd;
#if POLARSSL
    int listen_fd = si->fd;
#else
    int listen_fd = si->netctx.fd;
    mbedtls_net_context clientctx;
#endif
    int maxfd = listen_fd;

    FD_ZERO(&rfds_proto);
    FD_SET(listen_fd, &rfds_proto);
    FD_SET(si->exitPipe[0], &rfds_proto);
    if (si->exitPipe[0] > maxfd)
        maxfd = si->exitPipe[0];

    while (1)
    {
        rfds = rfds_proto;
        select(maxfd ,&rfds, NULL, NULL, NULL);
        if (FD_ISSET(listen_fd, &rfds))
        {
#if POLARSSL
            ret = net_accept(listen_fd, &fd, NULL);
#else
            ret = mbedtls_net_accept(&si->netctx, &clientctx,
                                     NULL, 0, NULL);
            fd = clientctx.fd;
#endif

            if (ret == 0)
            {
                _ProtoSSLConn * c = si->factory->newConnection();
                {
                    WaitUtil::Lock lck(&connLock);
                    conns[fd] = c;
                } // lock released here
#if POLARSSL
                if (c->_startThread(this, true, fd) == false)
#else
                if (c->_startThread(this, true, clientctx) == false)
#endif
                {
                    printf("start thread failed\n");
#if POLARSSL
                    net_close(fd);
#else
                    mbedtls_net_free(&clientctx);
#endif
                }
            }
            else
            {
                printf("accept returned shit\n");
            }
        }
        if (FD_ISSET(si->exitPipe[0], &rfds))
        {
            break;
        }
    }
#if POLARSSL
    net_close(si->fd);
#else
    mbedtls_net_free(&si->netctx);
#endif

    WaitUtil::Lock lck(&connLock);
    serverInfoMap::iterator it = servers.find(listen_fd);
    if (it != servers.end())
        servers.erase(it);
}

bool
ProtoSSLMsgs::startClient(ProtoSSLConnFactory &factory,
                          const std::string &remoteHost, int remotePort)
{
    int ret, fd;
    char strbuf[200];

#if !POLARSSL
    mbedtls_net_context clientctx;
#endif

#if POLARSSL
    ret = net_connect(&fd, remoteHost.c_str(), remotePort);
#else
    mbedtls_ssl_conf_endpoint( &sslcfg, MBEDTLS_SSL_IS_CLIENT );
    char portString[8];
    sprintf(portString,"%d", remotePort);
    ret = mbedtls_net_connect(&clientctx,
                              remoteHost.c_str(), portString,
                              MBEDTLS_NET_PROTO_TCP);
    fd = clientctx.fd;
#endif
    if (ret != 0)
    {
        mbedtls_strerror( ret, strbuf, sizeof(strbuf));
        printf("net connect returns 0x%x: %s\n", -ret, strbuf);
        return false;
    }

    _ProtoSSLConn * c = factory.newConnection();
    {
        WaitUtil::Lock lck(&connLock);
        conns[fd] = c;
    } // lock released here
#if POLARSSL
    if (c->_startThread(this, false, fd) == false)
#else
    if (c->_startThread(this, false, clientctx) == false)
#endif
    {
#if POLARSSL
        close(fd);
#else
        mbedtls_net_free(&clientctx);
#endif
        printf("start thread failed\n");
        return false;
    }

    return true;
}

bool
ProtoSSLMsgs::run(int timeout_ms /*= -1*/)
{
    fd_set  rfds, rfds_proto;
    struct timeval tv, tv_proto, *tvp;

    FD_ZERO(&rfds_proto);
    FD_SET(exitPipe[0], &rfds_proto);

    if (timeout_ms == -1)
    {
        tvp = NULL;
    }
    else
    {
        tvp = &tv;
        tv_proto.tv_sec = timeout_ms / 1000;
        tv_proto.tv_usec = (timeout_ms % 1000) * 1000;
    }
    
    while (1)
    {
        rfds = rfds_proto;
        tv = tv_proto;
        int cc = select(exitPipe[0]+1, &rfds, NULL, NULL, tvp);
        if (cc > 0)
        {
            if (FD_ISSET(exitPipe[0], &rfds))
                return false;
        }
        if (cc == 0)
            break;
    }

    return true;
}

void
ProtoSSLMsgs::stop(void)
{
    char dummy = 1;
    if (::write(exitPipe[1], &dummy, 1) < 0)
        printf("ProtoSSLMsgs::stop: "
               "write failed: %d: %s\n", errno, strerror(errno));
}

}; // namespace ProtoSSL
