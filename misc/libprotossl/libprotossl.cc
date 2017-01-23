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
#include <iostream>

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
    : fd(-1), rcvdMessage(_rcvdMessage), msgs(NULL)
{
    fd = -1;
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
    if (fd > 0)
    {
        net_close(fd);
        msgs->deregisterConn(fd,this);
        ssl_free(&sslctx);
    }
}

bool
_ProtoSSLConn::_startThread(ProtoSSLMsgs * _msgs, bool isServer, int _fd)
{
    int ret;

    msgs = _msgs;
    fd = _fd;

    if ((ret = ssl_init( &sslctx )) != 0)
    {
        printf("ssl init returned -0x%x\n", -ret);
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

    pipe(exitPipe);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&thread_id, &attr,
                   &_ProtoSSLConn::threadMain, (void*) this);
    pthread_attr_destroy(&attr);

    return true;
}

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

    while ((ret = ssl_handshake( &sslctx )) != 0)
    {
        if (ret != POLARSSL_ERR_NET_WANT_READ &&
            ret != POLARSSL_ERR_NET_WANT_WRITE )
        {
            printf( " failed\n  ! ssl_handshake returned -0x%x\n\n", -ret );
            goto bail;
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
        goto bail;
    }

    handleConnect();

    maxfd = fd;
    FD_ZERO(&rfds_proto);
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
            rcvbuf.resize(SSL_MAX_CONTENT_LEN);
            ret = ssl_read( &sslctx,
                            (unsigned char*) rcvbuf.c_str(),
                            SSL_MAX_CONTENT_LEN );
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
// if (ret == POLARSSL_ERR_NET_WANT_READ ||
//     ret == POLARSSL_ERR_NET_WANT_WRITE) { }
            if (ret == POLARSSL_ERR_SSL_PEER_CLOSE_NOTIFY ||
                ret == 0)
            {
                net_close(fd);
                fd = -1;
                if (ret == 0)
                    std::cout << "remote disconnect unclean\n";
                ssl_session_reset( &sslctx );
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
        ssl_close_notify( &sslctx );
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
        ret = ssl_write( &sslctx, reinterpret_cast<const unsigned char *>(
                             outbuf.c_str()), outbuf.length() );
    } while (ret == POLARSSL_ERR_NET_WANT_READ ||
             ret == POLARSSL_ERR_NET_WANT_WRITE);

    msg.Clear();
    outbuf.clear();

    if (ret < 0)
        std::cout << "ssl_write returned " << std::hex << -ret << std::endl;

    if (ret == POLARSSL_ERR_NET_CONN_RESET)
    {
        std::cout << "ssl_write peer closed connection" << std::endl;
        return false;
    }

    return false;
}

void
_ProtoSSLConn::closeConnection(void)
{
    char dummy = 1;
    ::write(exitPipe[1], &dummy, 1);
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
    entropy_init( &entropy );
    memset( &mycert, 0, sizeof( x509_crt ) );
    memset( &cacert, 0, sizeof( x509_crt ) );
    pk_init( &mykey ); // rsa_init( &mykey, RSA_PKCS_V15, 0 );
    pipe(exitPipe);
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
            write(si.exitPipe[1], &dummy, 1);
        }
    } // unlock

    // wait for all connection threads and server threads
    // to exit, call in, and remove them selves from the
    // maps as they exit.
    while ((conns.size() > 0) || (servers.size() > 0))
        usleep(1);

    // TODO pop servers list and clean
    // TODO pop connMap and clean
    pk_free( &mykey);
    x509_crt_free( &mycert );
    x509_crt_free( &cacert );
    close(exitPipe[0]);
    close(exitPipe[1]);
}

bool
ProtoSSLMsgs::loadCertificates(const ProtoSSLCertParams &params)
{
    int ret;
    static const char * pers = "ProtoSSLDRBG";

    if( ( ret = ctr_drbg_init( &ctr_drbg, entropy_func, &entropy,
                               (const unsigned char *) pers,
                               strlen( pers ) ) ) != 0 )
    {
        printf( " ctr_drbg_init returned -0x%x\n", -ret );
        return false;
    }

    otherCommonName = params.otherCommonName;

    if (params.caCert.compare(0,5,"file:") == 0)
        ret = x509_crt_parse_file( &cacert,
                                   params.caCert.c_str() + 5);
    else
        ret = x509_crt_parse( &cacert,
                              (const unsigned char *) params.caCert.c_str(),
                              params.caCert.size());
    if (ret != 0)
    {
        printf( " 1 x509parse_crt returned -0x%x\n\n", -ret );
        return false;
    }

    if (params.myCert.compare(0,5,"file:") == 0)
        ret = x509_crt_parse_file( &mycert,
                                   params.myCert.c_str()+5);
    else
        ret = x509_crt_parse( &mycert,
                              (const unsigned char *) params.myCert.c_str(),
                              params.myCert.size());
    if (ret != 0)
    {
        printf( " 2 x509parse_crt returned -0x%x\n\n", -ret );
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
        ret = pk_parse_keyfile( &mykey, 
                                params.myKey.c_str() + 5,
                                keyPassword);
    else
        ret = pk_parse_key( &mykey, 
                            (const unsigned char *) params.myKey.c_str(),
                            params.myKey.size(),
                            (const unsigned char *) keyPassword,
                            keyPasswordLen);
    if (ret != 0)
    {
        printf( " 3 pk_parse_keyfile returned -0x%x\n\n", -ret );
        return false;
    }

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
    int ret, fd;

    ret = net_bind(&fd, NULL, listeningPort);
    if (ret != 0)
    {
        printf("net bind returned -0x%x\n", -ret);
        return false;
    }

    WaitUtil::Lock lck(&connLock);
    serverInfo & si = servers[fd]; // note this creates new entry

    si.fd = fd;
    si.msgs = this;
    pipe(si.exitPipe);
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
    int maxfd = si->fd;
    int ret, fd;

    FD_ZERO(&rfds_proto);
    FD_SET(si->fd, &rfds_proto);
    FD_SET(si->exitPipe[0], &rfds_proto);
    if (si->exitPipe[0] > maxfd)
        maxfd = si->exitPipe[0];

    while (1)
    {
        rfds = rfds_proto;
        select(maxfd ,&rfds, NULL, NULL, NULL);
        if (FD_ISSET(si->fd, &rfds))
        {
            if ((ret = net_accept(si->fd, &fd, NULL)) == 0)
            {
                _ProtoSSLConn * c = si->factory->newConnection();
                {
                    WaitUtil::Lock lck(&connLock);
                    conns[fd] = c;
                } // lock released here
                if (c->_startThread(this, true, fd) == false)
                {
                    printf("start thread failed\n");
                    net_close(fd);
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
    net_close(si->fd);

    WaitUtil::Lock lck(&connLock);
    serverInfoMap::iterator it = servers.find(si->fd);
    if (it != servers.end())
        servers.erase(it);
}

bool
ProtoSSLMsgs::startClient(ProtoSSLConnFactory &factory,
                          const std::string &remoteHost, int remotePort)
{
    int ret, fd;

    if ((ret = net_connect(&fd, remoteHost.c_str(), remotePort)) != 0)
    {
        printf("net connect returns -0x%x\n", -ret);
        return false;
    }

    _ProtoSSLConn * c = factory.newConnection();
    {
        WaitUtil::Lock lck(&connLock);
        conns[fd] = c;
    } // lock released here
    if (c->_startThread(this, false, fd) == false)
    {
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
    ::write(exitPipe[1], &dummy, 1);
}

}; // namespace ProtoSSL
